/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Missing function declarations for Zephyr compatibility
 */

#pragma once

#include <zephyr/kernel.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

/* Forward declarations for c-rbtree types */
typedef struct CRBNode CRBNode;
typedef struct CRBTree CRBTree;

/* Forward declarations for controller */
typedef struct Controller Controller;
typedef struct Message Message;

/* POSIX function declarations */
int ioctl(int fd, unsigned long request, ...);
int signalfd(int fd, const sigset_t *mask, int flags);
int fchmod(int fd, mode_t mode);
struct passwd *getpwuid(uid_t uid);
int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups);

/* c-rbtree function declarations */
void c_rbnode_unlink_stale(CRBNode *n);
CRBNode *c_rbtree_first_postorder(CRBTree *t);
CRBNode *c_rbnode_next_postorder(CRBNode *n);
void c_rbtree_add(CRBTree *t, CRBNode *p, CRBNode **l, CRBNode *n);

/* controller function declarations */
int controller_dbus_dispatch(Controller *controller, Message *message);