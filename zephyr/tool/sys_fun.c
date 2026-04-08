/* Additional system function implementations for dbus-broker on Zephyr */

/* Must include Zephyr headers first to avoid type conflicts */
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/random/random.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "dbus_broker_zephyr_compat.h"

/* Program name for syslog compatibility - skip definition, use sys_compat.c definition */

/* strdup implementation */
// char *strdup(const char *s)
// {
//     /* Remove NULL check since function is marked nonnull */
//     if (0 && !s) {
//         return NULL;
//     }
    
//     size_t len = strlen(s) + 1;
//     char *copy = malloc(len);
//     if (copy) {
//         memcpy(copy, s, len);
//     }
    
//     return copy;
// }

/* strndup implementation */
// char *strndup(const char *s, size_t n)
// {
//     /* Remove NULL check since function is marked nonnull */
//     if (0 && !s) {
//         return NULL;
//     }
    
//     size_t len = strlen(s);
//     if (len > n) {
//         len = n;
//     }
    
//     char *copy = malloc(len + 1);
//     if (copy) {
//         memcpy(copy, s, len);
//         copy[len] = '\0';
//     }
    
//     return copy;
// }

/* getrandom implementation - use Zephyr's random number generator */
ssize_t getrandom(void *buf, size_t buflen, unsigned int flags)
{
    ARG_UNUSED(flags);
    
    if (!buf || buflen == 0) {
        errno = EINVAL;
        return -1;
    }
    
    /* Use Zephyr's random number generator */
    for (size_t i = 0; i < buflen; i++) {
        ((uint8_t *)buf)[i] = sys_rand8_get();
    }
    
    return buflen;
}

/* realpath implementation - simplified */
// char *realpath(const char *path, char *resolved_path)
// {
//     if (!path) {
//         errno = EINVAL;
//         return NULL;
//     }

//     char *result = resolved_path;
//     if (!result) {
//         result = malloc(PATH_MAX);
//         if (!result) {
//             errno = ENOMEM;
//             return NULL;
//         }
//     }

//     /* Simple implementation - just copy the path */
//     strncpy(result, path, PATH_MAX - 1);
//     result[PATH_MAX - 1] = '\0';

//     return result;
// }

/* mkstemp implementation - not supported */
int mkstemp(char *template)
{
    ARG_UNUSED(template);
    
    errno = ENOTSUP;
    return -1;
}

/* tempnam implementation - not supported */
char *tempnam(const char *dir, const char *pfx)
{
    ARG_UNUSED(dir);
    ARG_UNUSED(pfx);
    
    errno = ENOTSUP;
    return NULL;
}

/* Additional memory allocation functions */
void *valloc(size_t size)
{
    /* Just use malloc for Zephyr */
    return malloc(size);
}

// int posix_memalign(void **memptr, size_t alignment, size_t size)
// {
//     if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
//         return EINVAL;
//     }
    
//     void *ptr = malloc(size);
//     if (!ptr) {
//         return ENOMEM;
//     }
    
//     /* Simple alignment - may not work for all cases */
//     if (((uintptr_t)ptr & (alignment - 1)) != 0) {
//         free(ptr);
//         return ENOMEM;
//     }
    
//     *memptr = ptr;
//     return 0;
// }

/* inotify implementations - not supported in Zephyr */
int inotify_init(void)
{
    errno = ENOTSUP;
    return -1;
}

int inotify_init1(int flags)
{
    ARG_UNUSED(flags);
    errno = ENOTSUP;
    return -1;
}

int inotify_add_watch(int fd, const char *pathname, uint32_t mask)
{
    ARG_UNUSED(fd);
    ARG_UNUSED(pathname);
    ARG_UNUSED(mask);
    errno = ENOTSUP;
    return -1;
}

int inotify_rm_watch(int fd, int wd)
{
    ARG_UNUSED(fd);
    ARG_UNUSED(wd);
    errno = ENOTSUP;
    return -1;
}
