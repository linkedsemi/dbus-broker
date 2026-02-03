/* Compatibility layer for byteswap.h on Zephyr */

#ifndef BYTESWAP_H
#define BYTESWAP_H

#include <zephyr/sys/byteorder.h>

#define bswap_16(x) BSWAP_16(x)
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#endif /* BYTESWAP_H */
