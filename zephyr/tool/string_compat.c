/* String function compatibility implementations for dbus-broker on Zephyr */

#include <string.h>
#include <stdint.h>
#include "dbus_broker_zephyr_compat.h"

/* Custom implementations if builtin ones are not available */

#ifndef __BUILTIN_MEMSET
void *memset(void *ptr, int value, size_t num) {
    uint8_t *p = (uint8_t *)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (uint8_t)value;
    }
    return ptr;
}
#endif

#ifndef __BUILTIN_MEMCPY
void *memcpy(void *dest, const void *src, size_t num) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
    return dest;
}
#endif

#ifndef __BUILTIN_MEMCMP
int memcmp(const void *ptr1, const void *ptr2, size_t num) {
    const uint8_t *p1 = (const uint8_t *)ptr1;
    const uint8_t *p2 = (const uint8_t *)ptr2;
    for (size_t i = 0; i < num; i++) {
        if (p1[i] != p2[i]) {
            return (int)(p1[i]) - (int)(p2[i]);
        }
    }
    return 0;
}
#endif

#ifndef __BUILTIN_STRLEN
size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}
#endif

#ifndef __BUILTIN_STRCMP
int strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}
#endif

#ifndef __BUILTIN_STRNCMP
int strncmp(const char *str1, const char *str2, size_t num) {
    for (size_t i = 0; i < num; i++) {
        if (str1[i] != str2[i]) {
            return (int)(unsigned char)str1[i] - (int)(unsigned char)str2[i];
        }
        if (str1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}
#endif

#ifndef __BUILTIN_STRNCPY
char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}
#endif