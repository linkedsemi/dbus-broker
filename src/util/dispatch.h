#pragma once

/*
 * Event Dispatcher
 */

#include <c-list.h>
#include <c-stdaux.h>
#include <stdlib.h>

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <zephyr/posix/poll.h>
#else
#include <sys/epoll.h>
#endif

enum {
        _DISPATCH_E_SUCCESS,

        DISPATCH_E_EXIT,
        DISPATCH_E_FAILURE,
};

typedef struct DispatchContext DispatchContext;
typedef struct DispatchFile DispatchFile;
typedef int (*DispatchFn) (DispatchFile *file);

/* files */

struct DispatchFile {
        DispatchContext *context;
        CList ready_link;
        DispatchFn fn;

        int fd;
        uint32_t user_mask;
        uint32_t kernel_mask;
        uint32_t events;
};

#define DISPATCH_FILE_NULL(_x) {                                \
                .context = NULL, \
                .ready_link = C_LIST_INIT((_x).ready_link),     \
                .fn = NULL, \
                .fd = -1,                                       \
                .user_mask = 0, \
                .kernel_mask = 0, \
                .events = 0, \
        }

int dispatch_file_init(DispatchFile *file,
                       DispatchContext *ctx,
                       DispatchFn fn,
                       int fd,
                       uint32_t mask,
                       uint32_t events);
void dispatch_file_deinit(DispatchFile *file);

void dispatch_file_select(DispatchFile *file, uint32_t mask);
void dispatch_file_deselect(DispatchFile *file, uint32_t mask);
void dispatch_file_clear(DispatchFile *file, uint32_t mask);

/* contexts */

struct DispatchContext {
#ifdef __ZEPHYR__
        struct pollfd *fds;
        DispatchFile **files;
        size_t n_fds_allocated;
        size_t n_fds_used;
        CList ready_list;
        size_t n_files;
        int terminate_pipe[2];                  // Pipe for termination notification in poll
        int source;  // 0: broker poll, 1: sd-event
#else
        int epoll_fd;
        CList ready_list;
        size_t n_files;
#endif
};

#define DISPATCH_CONTEXT_NULL(_x) {                             \
                .n_files = 0, \
                .ready_list = C_LIST_INIT((_x).ready_list),     \
                .n_fds_allocated = 0, \
                .n_fds_used = 0, \
                .fds = NULL, \
                .files = NULL, \
        }

int dispatch_context_init(DispatchContext *ctx);
void dispatch_context_deinit(DispatchContext *ctx);

int dispatch_context_poll(DispatchContext *ctx, int timeout);
int dispatch_context_dispatch(DispatchContext *ctx);
void dispatch_context_terminate(DispatchContext *ctx);

/* inline helpers */

static inline uint32_t dispatch_file_events(DispatchFile *file) {
        return file->events & file->user_mask;
}
