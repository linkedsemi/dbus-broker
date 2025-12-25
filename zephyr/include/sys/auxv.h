/*
 * Virtual auxv.h for Zephyr
 * Provides compatibility definitions for auxv functionality used by dbus-broker
 */

#pragma once

#include "../dbus_broker_zephyr_compat.h"

/* Forward declarations for auxv functions */
unsigned long getauxval(unsigned long type);