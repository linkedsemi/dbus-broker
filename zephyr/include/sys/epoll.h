/*
 * Virtual epoll.h for Zephyr
 * Provides compatibility definitions for epoll functionality used by dbus-broker
 */

#pragma once

#include "../dbus_broker_zephyr_compat.h"

/* Forward declarations for epoll functions */
int epoll_create(int size);
int epoll_create1(int flags);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
int epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask);
