/* Additional missing functions for dbus-broker on Zephyr */

/* Must include Zephyr headers first to avoid type conflicts */
#include <zephyr/kernel.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/signal.h>

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include "dbus_broker_zephyr_compat.h"

/* signalfd implementation - not supported in Zephyr */
int signalfd(int fd, const sigset_t *mask, int flags)
{
    ARG_UNUSED(fd);
    ARG_UNUSED(mask);
    ARG_UNUSED(flags);
    
    errno = ENOTSUP;
    return -1;
}

/* ioctl implementation - minimal for Zephyr */
int ioctl(int fd, unsigned long request, ...)
{
    ARG_UNUSED(fd);
    ARG_UNUSED(request);
    
    errno = ENOTSUP;
    return -1;
}

/* fchmod implementation - not supported in Zephyr */
int fchmod(int fd, mode_t mode)
{
    ARG_UNUSED(fd);
    ARG_UNUSED(mode);
    
    errno = ENOTSUP;
    return -1;
}

/* passwd structure for getpwuid */
struct passwd {
    char *pw_name;
    char *pw_passwd;
    uid_t pw_uid;
    gid_t pw_gid;
    char *pw_gecos;
    char *pw_dir;
    char *pw_shell;
};

static struct passwd dummy_passwd = {
    .pw_name = "root",
    .pw_passwd = "",
    .pw_uid = 0,
    .pw_gid = 0,
    .pw_gecos = "root",
    .pw_dir = "/",
    .pw_shell = "/bin/sh"
};

struct passwd *getpwuid(uid_t uid)
{
    ARG_UNUSED(uid);
    return &dummy_passwd;
}

/* getgrouplist implementation - minimal */
int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups)
{
    ARG_UNUSED(user);
    
    if (!groups || !ngroups) {
        errno = EINVAL;
        return -1;
    }
    
    if (*ngroups < 1) {
        *ngroups = 1;
        errno = EINVAL;
        return -1;
    }
    
    groups[0] = group;
    return 1;
}

/*
 * c-rbtree function implementations - needed because _c_public_ visibility
 * attribute may not work correctly in Zephyr build environment
 */

/* Forward declaration for controller */
typedef struct Controller Controller;
typedef struct Message Message;