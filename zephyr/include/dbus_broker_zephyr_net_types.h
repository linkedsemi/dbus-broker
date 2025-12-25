/* Zephyr network type definitions for dbus-broker compatibility */

#pragma once

#include <stdint.h>

/* Basic network types needed before full definitions */
#ifndef sa_family_t
typedef unsigned short sa_family_t;
#endif

#ifndef socklen_t
typedef unsigned int socklen_t;
#endif

/* Forward declarations for network structures */
struct sockaddr;
struct msghdr;
struct in_addr;
struct in6_addr;
struct iovec;