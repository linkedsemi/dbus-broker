/* System compatibility implementations for dbus-broker on Zephyr */

/* Must include Zephyr headers first to avoid type conflicts */
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/signal.h>

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "dbus_broker_zephyr_compat.h"

/* Definition of program_invocation_short_name - only if not already defined */
#ifndef program_invocation_short_name
char *program_invocation_short_name = "dbus-broker";
#endif

/* readlinkat implementation - minimal for Zephyr */
ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz)
{
    ARG_UNUSED(dirfd);
    ARG_UNUSED(pathname);
    ARG_UNUSED(buf);
    ARG_UNUSED(bufsiz);
    
    errno = ENOTSUP;
    return -1;
}

/* pipe2 implementation - not supported in Zephyr */
int pipe2(int pipefd[2], int flags)
{
    ARG_UNUSED(pipefd);
    ARG_UNUSED(flags);
    
    errno = ENOTSUP;
    return -1;
}

/* accept4 implementation - fallback to accept */
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
    int fd = accept(sockfd, addr, addrlen);
    if (fd < 0) {
        return -1;
    }
    
    /* Set close-on-exec flag if requested */
    if (flags & SOCK_CLOEXEC) {
        fcntl(fd, F_SETFD, FD_CLOEXEC);
    }

    /*
     * Honor SOCK_NONBLOCK. dbus-broker is a single-threaded event
     * loop and strictly requires all peer fds to be non-blocking:
     * dropping this flag lets a slow client (full recv pipe) block
     * the broker in a write, stalling AUTH handshakes of every other
     * peer ("Bus not ready" timeouts on connect).
     */
    if (flags & SOCK_NONBLOCK) {
        int fl = fcntl(fd, F_GETFL, 0);

        if (fl < 0 || fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0) {
            close(fd);
            return -1;
        }
    }
    
    return fd;
}

/* memfd_create implementation - not supported in Zephyr */
int memfd_create(const char *name, unsigned int flags)
{
    ARG_UNUSED(name);
    ARG_UNUSED(flags);
    
    errno = ENOTSUP;
    return -1;
}

/* Skip mmap/munmap implementation - Zephyr provides these */

/* epoll implementation - not supported, return errors */
int epoll_create1(int flags)
{
    ARG_UNUSED(flags);
    
    errno = ENOTSUP;
    return -1;
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    ARG_UNUSED(epfd);
    ARG_UNUSED(op);
    ARG_UNUSED(fd);
    ARG_UNUSED(event);
    
    errno = ENOTSUP;
    return -1;
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
    ARG_UNUSED(epfd);
    ARG_UNUSED(events);
    ARG_UNUSED(maxevents);
    ARG_UNUSED(timeout);
    
    errno = ENOTSUP;
    return -1;
}

/* User/group operations - not supported in Zephyr */
int setgroups(size_t size, const gid_t *list)
{
    ARG_UNUSED(size);
    ARG_UNUSED(list);
    errno = ENOTSUP;
    return -1;
}

int setgid(gid_t gid)
{
    ARG_UNUSED(gid);
    errno = ENOTSUP;
    return -1;
}

int setuid(uid_t uid)
{
    ARG_UNUSED(uid);
    errno = ENOTSUP;
    return -1;
}

/* Skip getuid/getgid/getpid/geteuid/getegid implementation - Zephyr/basu provide these */

/* Skip usleep/clock_gettime/fcntl implementation - Zephyr provides these */

/* k_thread_get_id implementation */
int k_thread_get_id(const void *thread)
{
    return (int)(uintptr_t)thread;
}

/* c-stdaux compatibility functions */
int c_close(int fd)
{
    return close(fd);
}

void c_closep(int *fdp)
{
    if (fdp && *fdp >= 0) {
        close(*fdp);
        *fdp = -1;
    }
}