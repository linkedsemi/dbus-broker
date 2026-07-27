/*
 * Event Dispatcher
 *
 * This event dispatcher provides a simple wrapper around edge-triggered epoll.
 * It consists of a DispatchContext to represent the epoll-set, and a
 * DispatchFile for each file-descriptor added to that epoll-set. All events
 * are delivered in edge-triggered mode and cached in the DispatchFile. By
 * default, this means that we will get woken up for each event once, and cache
 * it. Since we don't use level-triggered mode, a continuous unhandled event
 * will not cause any further wakeups, unless the event is triggered by the
 * kernel again.
 *
 * On top of this edge-triggered mirror of the kernel space, we provide a
 * level-triggered callback mechanism. That is, on each dispatch-file you can
 * `select` and `deselect` events you're interested in. As long as an event is
 * selected, you will get notified of it in level-triggered mode (that is,
 * until you handled it). Since our cache is distinct from the kernel data, we
 * need explicit notification of when an event is handled. Therefore, you must
 * clear any event when you handled it. This usually means catching EAGAIN
 * and then clearing the event.
 *
 * Every DispatchFile has 3 event masks:
 *
 *     * kernel_mask: This mask is constant and must be provided at
 *                    initialization time. It describes the events that we
 *                    asked the kernel to report via epoll_ctl(2). For
 *                    performance reasons we never modify this mask. If there
 *                    ever arises a need to update this mask according to our
 *                    user-mask, this can be added later on.
 *
 *     * user_mask: This mask reflects the events that the user selected and
 *                  thus is interested in. It must always be a subset of
 *                  @kernel_mask. As long as an event is set in the event-mask
 *                  and in @user_mask, its callback will be invoked in a
 *                  level-triggered manner.
 *
 *     * events: This mask reflects the events the kernel signalled. That is,
 *               those events are always a subset of @kernel_mask and cached as
 *               soon as the kernel signalled them.
 *               You must explicitly clear events once you handled them. The
 *               kernel never tells us about falling edges, so we must detect
 *               them manually (usually via EAGAIN).
 */

#include <c-list.h>
#include <c-stdaux.h>
#include <stdlib.h>

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <zephyr/posix/poll.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/net/socket.h>
#else
#include <sys/epoll.h>
#endif

#include "util/dispatch.h"
#include "util/error.h"

// LOG_MODULE_DECLARE(DBUS_BROKER, LOG_LEVEL_INF);

/**
 * dispatch_file_init() - initialize dispatch file
 * @file:               dispatch file
 * @ctx:                dispatch context
 * @fn:                 callback function
 * @fd:                 file descriptor
 * @mask:               EPOLL* event mask
 * @events:             initial EPOLL* event mask before calling into the kernel
 *
 * This initializes a new dispatch-file and registers it with the given
 * dispatch-context. The file-descriptor @fd is added to the epoll-set of @ctx
 * with the event mask @mask.
 *
 * Note that all event handling is always edge-triggered. Hence, EPOLLET must
 * not be passed in @mask, but is added automatically. Furthermore, the event
 * mask in @mask is used to select kernel events for edge-triggered mode. To
 * actually get notified via your callback, you must select the user-mask via
 * dispatch_file_select().
 *
 * The file-descriptor @fd is *NOT* consumed by this function. That is, the
 * caller still owns it, and is responsible to close it when done. However, the
 * caller must make sure to call dispatch_file_deinit() *BEFORE* closing the
 * FD.
 *
 * Return: 0 on success, negative error code on failure.
 */
int dispatch_file_init(DispatchFile *file,
                       DispatchContext *ctx,
                       DispatchFn fn,
                       int fd,
                       uint32_t mask,
                       uint32_t events) {
#ifdef __ZEPHYR__
        // Expand arrays if needed.
        if (ctx->n_fds_used >= ctx->n_fds_allocated) {
            size_t new_size = ctx->n_fds_allocated ? ctx->n_fds_allocated * 2 : 8;
            struct pollfd *new_fds = realloc(ctx->fds, new_size * sizeof(struct pollfd));
            DispatchFile **new_files = realloc(ctx->files, new_size * sizeof(DispatchFile *));

            if (!new_fds || !new_files) {
                free(new_fds);
                free(new_files);
                return error_origin(-ENOMEM);
            }
            ctx->fds = new_fds;
            ctx->files = new_files;
            ctx->n_fds_allocated = new_size;
        }

        ctx->fds[ctx->n_fds_used].fd = fd;
        ctx->fds[ctx->n_fds_used].events = mask;
        ctx->fds[ctx->n_fds_used].revents = events;
        ctx->files[ctx->n_fds_used] = file;

        ctx->n_fds_used++;

#else
        int r;

        c_assert(!(mask & EPOLLET));
        c_assert(!(events & ~mask));

        r = epoll_ctl(ctx->epoll_fd,
                      EPOLL_CTL_ADD,
                      fd,
                      &(struct epoll_event) {
                                .events = mask | EPOLLET,
                                .data.ptr = file,
                      });
        if (r < 0)
                return error_origin(-errno);
#endif

        file->context = ctx;
        file->ready_link = (CList)C_LIST_INIT(file->ready_link);
        file->fn = fn;
        file->fd = fd;
        file->user_mask = events;  // Set initial user_mask from events parameter
        file->kernel_mask = mask;
        file->events = events;

        ++file->context->n_files;

#ifdef __ZEPHYR__
        // Wake up the poll thread AFTER setting file masks, so poll sees correct user_mask/kernel_mask
        // BUGFIX: terminated pipe write was previously before user_mask/kernel_mask were set,
        // causing the next poll to see uninitialized masks (both 0), meaning events=0x0 for new fds
        char byte = 1;
        ssize_t write_result = write(ctx->terminate_pipe[1], &byte, 1);
        if (write_result < 0) {
            printk("[dispatch] dispatch_file_init: failed to write to terminate_pipe, errno=%d\n", errno);
        } else {
        //     printk("[dispatch] dispatch_file_init: woke up poll thread AFTER setting masks, user_mask=0x%x, kernel_mask=0x%x\n",
        //             file->user_mask, file->kernel_mask);
        }
#endif

        return 0;
}

/**
 * dispatch_file_deinit() - deinitialize dispatch file
 * @file:               dispatch file
 *
 * This deinitialized the dispatch-file @file and unregisters it from its
 * context. The file is put into a deinitialized state, hence, it is safe to
 * call this function multiple times.
 *
 * The file-descriptor provided via dispatch_file_init() is *NOT* closed, but
 * left unchanged. However, the caller must make sure to call
 * dispatch_file_deinit() *BEFORE* closing the FD.
 */
void dispatch_file_deinit(DispatchFile *file) {
        if (!file->context) {
                printk("[dispatch] dispatch_file_deinit: file->context is NULL, nothing to do\n");
                return;
        }
        
#ifdef __ZEPHYR__
        // Find and remove from the poll array
        bool found = false;
        for (size_t i = 0; i < file->context->n_fds_used; i++) {
            if (file->context->files[i] == file) {
                found = true;
                
                // Move last element to current position to fill gap
                if (i < file->context->n_fds_used - 1) {
                    file->context->fds[i] = file->context->fds[file->context->n_fds_used - 1];
                    file->context->files[i] = file->context->files[file->context->n_fds_used - 1];
                }
                file->context->n_fds_used--;
                
                // Clear ready link
                c_list_unlink(&file->ready_link);
                --file->context->n_files;
                
                // printk("[dispatch] dispatch_file_deinit: removed file=%p, fd=%d, n_files=%zu, n_fds_used=%zu", 
                //         file, file->fd, file->context->n_files, file->context->n_fds_used);
                break;
            }
        }
        
        if (!found) {
            printk("[dispatch] dispatch_file_deinit: file=%p not found in context! n_files=%zu, n_fds_used=%zu\n", 
                    file, file->context->n_files, file->context->n_fds_used);
        }
#else
                int r;

                r = epoll_ctl(file->context->epoll_fd, EPOLL_CTL_DEL, file->fd, NULL);
                c_assert(r >= 0);

                --file->context->n_files;
                c_list_unlink(&file->ready_link);
#endif

        file->fd = -1;
        file->fn = NULL;
        file->context = NULL;
}

/**
 * dispatch_file_select() - select notification mask
 * @file:               dispatch file
 * @mask:               event mask
 *
 * This selects the events specified in @mask for notification. That is, if
 * those events are signalled by the kernel, the callback of @file will be
 * invoked for those events.
 *
 * Once you lost interest in a given event, you must deselect it via
 * dispatch_file_deselect(). Otherwise, you will keep being notified of the
 * event.
 *
 * Once you handled an event fully, you must clear it via dispatch_file_clear()
 * to tell the dispatcher that you should only be invoked for the event
 * when the kernel signals it again.
 */
void dispatch_file_select(DispatchFile *file, uint32_t mask) {
        c_assert(!(mask & ~file->kernel_mask));

        file->user_mask |= mask;
        if ((file->user_mask & file->events) && !c_list_is_linked(&file->ready_link))
                c_list_link_tail(&file->context->ready_list, &file->ready_link);
}

/**
 * dispatch_file_deselect() - deselect notification mask
 * @file:               dispatch file
 * @mask:               event mask
 *
 * This is the inverse of dispatch_file_select() and removes a given event mask
 * from the user-mask. The callback will no longer be invoked for those events.
 */
void dispatch_file_deselect(DispatchFile *file, uint32_t mask) {
        c_assert(!(mask & ~file->kernel_mask));

        file->user_mask &= ~mask;
        if (!(file->events & file->user_mask))
                c_list_unlink(&file->ready_link);
}

/**
 * dispatch_file_clear() - clear kernel event mask
 * @file:               dispatch file
 * @mask:               event mask
 *
 * This clears the events in @mask from the pending kernel event mask. That is,
 * those events are now considered as 'handled'. The kernel must notify us of
 * them again to reconsider them.
 */
void dispatch_file_clear(DispatchFile *file, uint32_t mask) {
        c_assert(!(mask & ~file->kernel_mask));

        file->events &= ~mask;
        if (!(file->events & file->user_mask))
                c_list_unlink(&file->ready_link);
}

/**
 * dispatch_context_init() - initialize dispatch context
 * @ctx:                dispatch context
 *
 * This initializes a new dispatch context.
 *
 * Return: 0 on success, negative error code on failure.
 */
int dispatch_context_init(DispatchContext *ctx) {
        *ctx = (DispatchContext)DISPATCH_CONTEXT_NULL(*ctx);

#ifdef __ZEPHYR__
        // Allocate initial space for pollfd structures
        ctx->fds = calloc(8, sizeof(struct pollfd));
        ctx->files = calloc(8, sizeof(DispatchFile *));
        
        if (!ctx->fds || !ctx->files) {
            free(ctx->fds);
            free(ctx->files);
            return error_origin(-ENOMEM);
        }
        ctx->n_fds_allocated = 8;

        // Initialize termination pipe
        int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, ctx->terminate_pipe);
        if (ret < 0) {
            printk("[dispatch] socketpair failed: %d\n", errno);
            free(ctx->fds);
            free(ctx->files);
            return error_origin(-errno);
        }
        // printk("Termination pipe initialized: [%d, %d]", ctx->terminate_pipe[0], ctx->terminate_pipe[1]);
#else
        ctx->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (ctx->epoll_fd < 0)
                return error_origin(-errno);
#endif

        return 0;
}

/**
 * dispatch_context_deinit() - deinitialize dispatch context
 * @ctx:                dispatch context
 *
 * This deinitializes a dispatch context. The caller must make sure no
 * dispatch-file is registered on it.
 *
 * The context will be set into an deinitialized state afterwards. Hence, it is
 * safe to call this function multiple times.
 */
void dispatch_context_deinit(DispatchContext *ctx) {
        // printk("[dispatch] dispatch_context_deinit: n_files=%zu, n_fds_used=%zu, ready_list_empty=%d", 
        //         ctx->n_files, ctx->n_fds_used, c_list_is_empty(&ctx->ready_list));
        
        c_assert(!ctx->n_files);
        c_assert(c_list_is_empty(&ctx->ready_list));

#ifdef __ZEPHYR__
        free(ctx->fds);
        free(ctx->files);
        ctx->fds = NULL;
        ctx->files = NULL;

        // Close termination pipes
        if (ctx->terminate_pipe[0] != -1) {
            close(ctx->terminate_pipe[0]);
        }
        if (ctx->terminate_pipe[1] != -1) {
            close(ctx->terminate_pipe[1]);
        }

        ctx->n_fds_allocated = 0;
        ctx->n_fds_used = 0;
#else
        ctx->epoll_fd = c_close(ctx->epoll_fd);
#endif
}

/**
 * dispatch_context_poll() - fetch events from kernel
 * @ctx:                dispatch context
 * @timeout:            poll timeout
 *
 * This calls into epoll_wait(2) to fetch events on all registered
 * dispatch-files from the kernel. @timeout is passed unmodified to
 * epoll_wait().
 *
 * The events fetched from the kernel are merged into our list of
 * dispatch-files. Nothing is dispatched! The data is merely fetched from the
 * kernel.
 *
 * Return: 0 on success, negative error code on failure.
 */
int dispatch_context_poll(DispatchContext *ctx, int timeout) {
#ifdef __ZEPHYR__
    size_t total_fds = ctx->n_fds_used;
    
    // Check if arrays are valid
    if (total_fds > 0 && (!ctx->fds || !ctx->files)) {
        printk("[dispatch] dispatch_context_poll: n_fds_used=%zu but fds or files is NULL!\n", total_fds);
        return error_origin(-EFAULT);
    }
    
    struct pollfd *temp_fds = malloc((total_fds + 1) * sizeof(struct pollfd));
    DispatchFile **temp_files = malloc((total_fds + 1) * sizeof(DispatchFile *));
    if (!temp_fds || !temp_files) {
        free(temp_fds);
        free(temp_files);
        return error_origin(-ENOMEM);
    }

    // 复制原始的 fds 和 files 指针，但根据 user_mask 设置要监听的事件
    for (size_t i = 0; i < total_fds; i++) {
        if (!ctx->files[i]) {
            printk("[dispatch] dispatch_context_poll: files[%u] is NULL!\n", i);
            free(temp_fds);
            free(temp_files);
            return error_origin(-EFAULT);
        }
        temp_fds[i].fd = ctx->fds[i].fd;
        temp_fds[i].events = ctx->files[i]->user_mask;  // 监听 user_mask 中的事件
        temp_fds[i].revents = 0;
        temp_files[i] = ctx->files[i];  // 复制指针
    }

    // Add terminate_pipe[0] to poll set for wake-up notifications
    temp_fds[total_fds].fd = ctx->terminate_pipe[0];
    temp_fds[total_fds].events = POLLIN;
    temp_fds[total_fds].revents = 0;
    temp_files[total_fds] = NULL;  // No DispatchFile for terminate_pipe
    
    // Validate all FDs before calling poll
    for (size_t i = 0; i < total_fds + 1; i++) {
        if (temp_fds[i].fd < 0) {
            printk("[dispatch] dispatch_context_poll: Invalid fd at index %zu: fd=%d\n", i, temp_fds[i].fd);
            free(temp_fds);
            free(temp_files);
            return error_origin(-EINVAL);
        }
    }
    
//     printk("[dispatch] poll called: total_fds=%zu, timeout=%d, terminate_pipe[0]=%d", 
//             total_fds, timeout, ctx->terminate_pipe[0]);

    k_msleep(5);
    int result = poll(temp_fds, total_fds + 1, timeout);
//     printk("[dispatch] poll returned: result=%d, errno=%d", result, errno);
    
    if (result < 0) {
        free(temp_fds);
        free(temp_files);
        if (errno == EINTR)
            return 0;
        printk("[dispatch] dispatch_context_poll: poll error, errno=%d\n", errno);
        return error_origin(-errno);
    }

    // Clear the terminate_pipe by reading once
    // Note: Zephyr's socketpair recv doesn't support MSG_DONTWAIT, so we only read once
    if (temp_fds[total_fds].revents & POLLIN) {
        char buffer[32];
        ssize_t read_bytes = recv(ctx->terminate_pipe[0], buffer, sizeof(buffer), 0);
        if (read_bytes <= 0) {
            printk("[dispatch] dispatch_context_poll: recv returned %zd, errno=%d\n", read_bytes, errno);
        }
    }

    // 处理结果并更新文件事件
    // 注意：poll 是电平触发，不同于 epoll 的边沿触发
    // 对于 poll，revents 表示的是当前状态，而不是边沿事件
    // 因此我们只关心 user_mask 中请求的事件，并直接用 revents 的对应位来更新
    for (size_t i = 0; i < total_fds; i++) {
        DispatchFile *f = temp_files[i];

        // 检查 NULL 指针和野指针
        if (!f || f->fd < 0) {
            printk("[dispatch] dispatch_context_poll: fd[%u] has invalid DispatchFile\n", i);
            continue;
        }

        // 验证 fd 匹配
        if (temp_fds[i].fd != f->fd) {
            printk("[dispatch] dispatch_context_poll: fd mismatch! temp_fds[%u].fd=%d, files[%u]->fd=%d\n",
                    i, temp_fds[i].fd, i, f->fd);
            continue;  // 跳过不匹配的
        }

        // 验证 context 指针
        if (f->context != ctx) {
            printk("[dispatch] dispatch_context_poll: fd[%u] DispatchFile has wrong context\n", i);
            continue;
        }

        if (temp_fds[i].revents) {
            // 只保留在 user_mask 中的事件
            uint32_t new_events = temp_fds[i].revents & f->kernel_mask & f->user_mask;
            f->events = new_events;  // 直接赋值，不累加，因为 poll 是电平触发
            if (f->events && !c_list_is_linked(&f->ready_link))
                c_list_link_tail(&f->context->ready_list, &f->ready_link);
        } else {
            // 如果poll没有返回事件，清除这个fd的所有旧events
            f->events = 0;
            // 如果 fd 不再有事件，从 ready_list 中移除
            if (c_list_is_linked(&f->ready_link))
                c_list_unlink(&f->ready_link);
        }
    }

    free(temp_fds);
    free(temp_files);

#else
        _c_cleanup_(c_freep) void *buffer = NULL;
        struct epoll_event *events, *e;
        DispatchFile *f;
        size_t n;
        int r;

        n = ctx->n_files * sizeof(*events);
        if (n > 128UL * 1024UL) {
                buffer = malloc(n);
                if (!buffer)
                        return error_origin(-ENOMEM);

                events = buffer;
        } else {
                events = alloca(n);
        }

        r = epoll_wait(ctx->epoll_fd, events, ctx->n_files, timeout);
        if (r < 0) {
                if (errno == EINTR)
                        return 0;

                return error_origin(-errno);
        }

        while (r > 0) {
                e = &events[--r];
                f = e->data.ptr;

                c_assert(f->context == ctx);

                f->events |= e->events & f->kernel_mask;
                if ((f->events & f->user_mask) && !c_list_is_linked(&f->ready_link))
                        c_list_link_tail(&f->context->ready_list, &f->ready_link);
        }

#endif
        return 0;
}

/**
 * dispatch_context_dispatch() - dispatch pending events
 * @ctx:                dispatch context
 *
 * This runs one dispatch round on the given dispatch context. That is, it
 * dispatches all pending events and calls into the callbacks of the respective
 * dispatch-file.
 *
 * The first non-zero return code of any dispatch-file callback will break the
 * loop and cause a propagation of that error code to the caller.
 *
 * Return: 0 on success, otherwise the first non-zero return code of any
 *         dispatched file stops dispatching and is returned unmodified.
 */
int dispatch_context_dispatch(DispatchContext *ctx) {
        CList todo = (CList)C_LIST_INIT(todo);
        DispatchFile *file;
        int r;

        // printk("[dispatch] dispatch_context_dispatch ENTER, ready_list empty=%d, n_fds_used=%zu, source=%d\n", 
        //         c_list_is_empty(&ctx->ready_list), ctx->n_fds_used, ctx->source);

        /* Use non-blocking poll (timeout=0) to avoid blocking in dispatch phase.
         * The blocking should happen in sd_event_wait, not here. */
        r = dispatch_context_poll(ctx, ctx->source ? 0 :
#ifdef __ZEPHYR__
                                  100   /* Zephyr: 100ms timeout avoids blocking
                                         * forever when no POLLOUT is signaled */
#else
                                  -1
#endif
                                  );
        
        // printk("[dispatch] dispatch_context_poll returned %d\n", r);
        
        if (r)
                return error_fold(r);

        /*
         * We want to dispatch @ctx->ready_list exactly once here. The trivial
         * approach would be to iterate it via c_list_for_each(). However, we
         * want to allow callbacks to modify their event masks, so we must
         * allow them to add and remove files arbitrarily. At the same time, we
         * want to prevent dispatching a single file twice, so we must make
         * sure to detect detach+reattach cycles to avoid starvation.
         *
         * Therefore, we simply fetch the entire ready-list into @todo and
         * handle it one-by-one, moving them back onto the ready-list. This is
         * safe against entry-removal in the callbacks, and it has a clearly
         * determined runtime.
         */
        c_list_swap(&todo, &ctx->ready_list);

        while ((file = c_list_first_entry(&todo, DispatchFile, ready_link))) {
                // printk("dispatch_context_dispatch: About to dispatch file=%p\n", file);
                if (!file) {
                        printk("[dispatch] dispatch_context_dispatch: file is NULL!\n");
                        break;
                }
                if (file->fd < 0) {
                        // printk("dispatch_context_dispatch: file->fd is invalid: %d\n", file->fd);
                        c_list_unlink(&file->ready_link);
                        continue;
                }
                if (!file->fn) {
                        printk("[dispatch] dispatch_context_dispatch: file->fn is NULL!\n");
                        c_list_unlink(&file->ready_link);
                        continue;
                }
                if (file->context != ctx) {
                        printk("[dispatch] dispatch_context_dispatch: file->context mismatch: file=%p, ctx=%p\n", file->context, ctx);
                        c_list_unlink(&file->ready_link);
                        continue;
                }
                c_list_unlink(&file->ready_link);

                r = file->fn(file);
                if (error_trace(r)) {
                        c_list_splice(&ctx->ready_list, &todo);
                        break;
                }

                /* Only re-add to ready_list if there are pending events to handle.
                 * This prevents endless loops when all events have been cleared. */
                if (file->events & file->user_mask) {
                        c_list_link_tail(&ctx->ready_list, &file->ready_link);
                }
        }

        c_assert(c_list_is_empty(&todo));
        // printk("[dispatch] dispatch_context_dispatch returning %d, ready_list empty=%d\n", r, 
        //         c_list_is_empty(&ctx->ready_list));
        return r;
}

/**
 * dispatch_context_terminate() - signal termination
 */
void dispatch_context_terminate(DispatchContext *ctx) {
#ifdef __ZEPHYR__
    // 发送终止信号到管道
    char byte = 1;
    write(ctx->terminate_pipe[1], &byte, 1);  // 写入到写端
#else
    // On Linux, we might send a signal to interrupt the main loop
    // This is typically handled by signalfd in the original code
    // For direct termination, we could use a pipe write or similar
    // For now, we'll leave it empty as the original signalfd mechanism
    // handles termination differently
#endif
}
