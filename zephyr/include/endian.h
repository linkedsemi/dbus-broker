/* Endian compatibility header for dbus-broker on Zephyr */

#ifndef __ENDIAN_H__
#define __ENDIAN_H__

#include <zephyr/sys/byteorder.h>
#include <zephyr/toolchain.h>

/* Define standard endian macros */
#if defined(CONFIG_BIG_ENDIAN)
#define __BYTE_ORDER __BIG_ENDIAN
#else
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif

#define __BIG_ENDIAN 4321
#define __LITTLE_ENDIAN 1234

/* Map Zephyr byte swap functions to standard names */
#ifndef htole16
#define htole16(x) sys_cpu_to_le16((uint16_t)(x))
#endif
#ifndef htole32
#define htole32(x) sys_cpu_to_le32((uint32_t)(x))
#endif
#ifndef htole64
#define htole64(x) sys_cpu_to_le64((uint64_t)(x))
#endif

#ifndef le16toh
#define le16toh(x) sys_le16_to_cpu((uint16_t)(x))
#endif
#ifndef le32toh
#define le32toh(x) sys_le32_to_cpu((uint32_t)(x))
#endif
#ifndef le64toh
#define le64toh(x) sys_le64_to_cpu((uint64_t)(x))
#endif

#ifndef htobe16
#define htobe16(x) sys_cpu_to_be16((uint16_t)(x))
#endif
#ifndef htobe32
#define htobe32(x) sys_cpu_to_be32((uint32_t)(x))
#endif
#ifndef htobe64
#define htobe64(x) sys_cpu_to_be64((uint64_t)(x))
#endif

#ifndef be16toh
#define be16toh(x) sys_be16_to_cpu((uint16_t)(x))
#endif
#ifndef be32toh
#define be32toh(x) sys_be32_to_cpu((uint32_t)(x))
#endif
#ifndef be64toh
#define be64toh(x) sys_be64_to_cpu((uint64_t)(x))
#endif

#endif /* __ENDIAN_H__ */