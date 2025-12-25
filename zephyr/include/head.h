/* Core compatibility header for dbus-broker on Zephyr */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* Ensure basic C library functions are available */
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <time.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

/* Include the main compatibility header after all basic includes */
#ifdef __ZEPHYR__
#include "dbus_broker_zephyr_compat.h"
#endif
#include <zephyr/logging/log.h>

#include "config.h"
// #include "dbus_broker_zephyr_compat.h"

/* Provide sys/syscall.h compatibility for Zephyr */
#ifdef DBUS_BROKER_BUILD_ZEPHYR
#include "sys_syscall.h"
#endif

/* Additional required includes for dbus-broker */
#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/fs/fs.h>

/* Function declaration fixes */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"