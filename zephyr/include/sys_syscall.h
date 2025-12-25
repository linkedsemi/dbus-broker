/* Compatibility layer for sys/syscall.h in Zephyr */

#ifndef SYS_SYSCALL_H
#define SYS_SYSCALL_H

#include <errno.h>

/* System call numbers for Zephyr compatibility */
#define SYS_getpid 0
#define SYS_gettid 1

/* System call wrapper functions */
static inline long syscall(long number, ...) {
    (void)number; /* Suppress unused parameter warning */
    errno = ENOSYS; /* System call not implemented */
    return -1;
}

#endif /* SYS_SYSCALL_H */