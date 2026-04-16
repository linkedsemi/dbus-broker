# SPDX-License-Identifier: Apache-2.0
#
# dbus-broker - High-performance D-Bus message broker for Zephyr
# This CMakeLists.txt provides modular configuration for the dbus-broker library

cmake_minimum_required(VERSION 3.20.0)

# Check if dbus-broker is enabled via Kconfig
if(NOT CONFIG_DBUS_BROKER)
  return()
endif()

zephyr_library_named(dbus_broker)

# ============================================================================
# Compilation Options
# ============================================================================
zephyr_library_compile_options(
  -msmall-data-limit=0
  -Wno-redundant-decls
  -Wno-nested-externs
)

# Force include compatibility header
target_compile_options(dbus_broker PRIVATE
  -include ${CMAKE_CURRENT_SOURCE_DIR}/zephyr/include/head.h
)

# ============================================================================
# Include Directories
# ============================================================================
zephyr_library_include_directories(
  # Public headers
  ${CMAKE_CURRENT_SOURCE_DIR}/zephyr/include
  ${CMAKE_CURRENT_SOURCE_DIR}/zephyr/include/sys
  ${CMAKE_CURRENT_SOURCE_DIR}/zephyr/include/linux
  
  # Internal headers
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/broker
  ${CMAKE_CURRENT_SOURCE_DIR}/src/bus
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dbus
  ${CMAKE_CURRENT_SOURCE_DIR}/src/util
  ${CMAKE_CURRENT_SOURCE_DIR}/src/catalog
  
  # Subproject dependencies
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcdvar-1/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libclist-3/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcrbtree-3/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcstdaux-1/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcutf8-1/src
)

# Also add subproject includes globally for dependent modules
target_include_directories(dbus_broker PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcstdaux-1/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcrbtree-3/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libclist-3/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcdvar-1/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcutf8-1/src
)

# ============================================================================
# Compile Definitions
# ============================================================================
zephyr_library_compile_definitions(
  __ZEPHYR__=1
  DBUS_BROKER_BUILD_ZEPHYR=1
  _FILE_OFFSET_BITS=64
  _GNU_SOURCE
  __LINUX_ERRNO_EXTENSIONS__
  _POSIX_C_SOURCE=200809L
  _XOPEN_SOURCE=700
  _DEFAULT_SOURCE
  PACKAGE_VERSION=37
  BINDIR="/usr/bin"
  SYSTEM_CONSOLE_USERS=""
  _TIME_BITS=64
  
  # Memory allocation configuration
  ENABLE_DEBUG_HASHMAP=0
  GPERF_LEN_TYPE=size_t
)

# Add log level definition from Kconfig
if(CONFIG_DBUS_BROKER_LOG_LEVEL EQUAL 0)
  zephyr_library_compile_definitions(LOG_LEVEL=0)
elseif(CONFIG_DBUS_BROKER_LOG_LEVEL EQUAL 1)
  zephyr_library_compile_definitions(LOG_LEVEL=1)
elseif(CONFIG_DBUS_BROKER_LOG_LEVEL EQUAL 2)
  zephyr_library_compile_definitions(LOG_LEVEL=2)
elseif(CONFIG_DBUS_BROKER_LOG_LEVEL EQUAL 3)
  zephyr_library_compile_definitions(LOG_LEVEL=3)
elseif(CONFIG_DBUS_BROKER_LOG_LEVEL EQUAL 4)
  zephyr_library_compile_definitions(LOG_LEVEL=4)
endif()

# Feature flags based on Kconfig
if(NOT CONFIG_DBUS_BROKER_ENABLE_POLICY)
  zephyr_library_compile_definitions(CONFIG_BROKER_ENABLE_POLICY=0)
endif()

if(NOT CONFIG_DBUS_BROKER_ENABLE_LAUNCH)
  zephyr_library_compile_definitions(CONFIG_BROKER_ENABLE_LAUNCH=0)
endif()

# Always disable unsupported features in Zephyr
zephyr_library_compile_definitions(
  CONFIG_BROKER_ENABLE_APPARMOR=0
  CONFIG_BROKER_ENABLE_AUDIT=0
  CONFIG_BROKER_ENABLE_SELINUX=0
)

# ============================================================================
# Zephyr Compatibility Layer
# ============================================================================
zephyr_library_sources(
  zephyr/tool/flockfile.c
  zephyr/tool/sys_compat.c
  zephyr/tool/sys_fun.c
  zephyr/tool/program_invocation.c
  zephyr/tool/missing_functions.c
  zephyr/tool/string_compat.c
)

# ============================================================================
# Subproject Dependencies
# ============================================================================

# c-rbtree - Red-black tree implementation
zephyr_library_sources(
  subprojects/libcrbtree-3/src/c-rbtree.c
)

# ============================================================================
# Core Source Files - Organized by Module
# ============================================================================

# --- Utility Library ---
zephyr_library_sources(
  src/util/error.c
  src/util/log.c
  src/util/misc.c
  src/util/string.c
  src/util/user.c
  src/util/fdlist.c
  src/util/dispatch.c
  src/util/fs.c
  src/util/dirwatch.c
  src/util/nsec.c
  src/util/proc.c
  src/util/sampler.c
  src/util/sockopt.c
  src/util/systemd.c
)

# --- Fallback Implementations (for disabled features) ---
zephyr_library_sources(
  src/util/apparmor-fallback.c
  src/util/audit-fallback.c
  src/util/selinux-fallback.c
)

# --- D-Bus Protocol Layer ---
zephyr_library_sources(
  src/dbus/message.c
  src/dbus/protocol.c
  src/dbus/socket.c
  src/dbus/connection.c
  src/dbus/queue.c
  src/dbus/address.c
  src/dbus/sasl.c
)

# --- Bus Management Layer ---
zephyr_library_sources(
  src/bus/bus.c
  src/bus/peer.c
  src/bus/name.c
  src/bus/match.c
  src/bus/reply.c
  src/bus/metrics.c
  src/bus/activation.c
  src/bus/diag.c
  src/bus/driver.c
  src/bus/listener.c
)

# Policy support (conditional)
if(CONFIG_DBUS_BROKER_ENABLE_POLICY)
  zephyr_library_sources(
    src/bus/policy.c
  )
endif()

# --- Broker Core ---
zephyr_library_sources(
  src/broker/broker.c
  src/broker/controller.c
  src/broker/controller-dbus.c
)

# Main entry point (only if building standalone)
if(CONFIG_DBUS_BROKER_STANDALONE)
  zephyr_library_sources(
    src/broker/main.c
  )
endif()