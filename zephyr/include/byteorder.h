/* Byteswap compatibility header for dbus-broker on Zephyr */

#include <zephyr/sys/byteorder.h>

#ifndef __BYTESWAP_H__
#define __BYTESWAP_H__

#ifndef bswap_16
#define bswap_16(x) BSWAP_16(x)
#endif
#ifndef bswap_32
#define bswap_32(x) BSWAP_32(x)
#endif
#ifndef bswap_64
#define bswap_64(x) BSWAP_64(x)
#endif

#endif /* __BYTESWAP_H__ */