#pragma once

/*
 * Compatibility header for sys/syscall.h in Zephyr
 * Provides minimal syscall compatibility for dbus-broker
 */

#include <unistd.h>

/* Define basic syscall numbers for RISC-V architecture */
#ifdef __riscv
#define __NR_memfd_create 279
#define __NR_pidfd_open 434
#endif

/* syscall function should be defined by the system or elsewhere */