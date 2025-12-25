#pragma once

/*
 * Compatibility header for linux/sockios.h in Zephyr
 * Provides minimal socket IOCTL compatibility for dbus-broker
 */

/* Socket IOCTLs commonly used in Linux */
#ifndef SIOCOUTQ
#define SIOCINQ      FIONREAD  /* Receive queue length */
#define SIOCOUTQ     TIOCOUTQ  /* Send queue length */
#endif
#define SIOCADDRT    0x890B    /* Add routing table entry */
#define SIOCDELRT    0x890C    /* Delete routing table entry */
#define SIOCGIFNAME  0x8910    /* Get interface name */
#define SIOCSIFLINK  0x8911    /* Set interface link */
#define SIOCGIFCONF  0x8912    /* Get interface list */
#define SIOCGIFFLAGS 0x8913    /* Get interface flags */
#define SIOCSIFFLAGS 0x8914    /* Set interface flags */
#define SIOCGIFADDR  0x8915    /* Get interface address */
#define SIOCSIFADDR  0x8916    /* Set interface address */

/* Additional socket constants */
#define TIOCOUTQ     0x5411    /* Get output queue length */
#define SCM_RIGHTS   0x01      /* Rights passed in message */