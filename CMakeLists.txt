# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

zephyr_library_named(dbus_broker)
zephyr_library_compile_options(-msmall-data-limit=0 -Wno-redundant-decls -Wno-nested-externs)

# Include directories
zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/zephyr/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/broker
  ${CMAKE_CURRENT_SOURCE_DIR}/src/bus
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dbus
  ${CMAKE_CURRENT_SOURCE_DIR}/src/util
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcdvar-1/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libclist-3/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcrbtree-3/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcstdaux-1/src
  ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libcutf8-1/src
)

# Compile definitions for Zephyr compatibility
zephyr_library_compile_definitions(
  __ZEPHYR__=1
  _FILE_OFFSET_BITS=64
  _GNU_SOURCE
  __LINUX_ERRNO_EXTENSIONS__
  
  # Disable features not supported by Zephyr
  CONFIG_BROKER_ENABLE_POLICY=0
  CONFIG_BROKER_ENABLE_LAUNCH=0
  CONFIG_BROKER_ENABLE_APPARMOR=0
  CONFIG_BROKER_ENABLE_AUDIT=0
  CONFIG_BROKER_ENABLE_SELINUX=0
  
  # Memory allocation configuration
  ENABLE_DEBUG_HASHMAP=0
  GPERF_LEN_TYPE=size_t
)

# Zephyr compatibility sources
zephyr_library_sources(
  zephyr/tool/flockfile.c
  zephyr/tool/sys_compat.c
  zephyr/tool/sys_fun.c
  zephyr/tool/program_invocation.c
  zephyr/tool/missing_functions.c
)

# c-rbtree dependency
zephyr_library_sources(
  subprojects/libcrbtree-3/src/c-rbtree.c
)

# Util - 基础工具库
zephyr_library_sources(
  src/util/error.c
  src/util/log.c
  src/util/misc.c
  src/util/string.c
  src/util/ref.h
  src/util/syscall.h
  src/util/common.h
  src/util/fdlist.c
  src/util/dispatch.c
  src/util/user.c
  src/util/fs.c
  src/util/dirwatch.c
  src/util/dirwatch.h
  src/util/nsec.c
  src/util/nsec.h
  src/util/proc.c
  src/util/proc.h
  src/util/sampler.c
  src/util/sampler.h
  src/util/sockopt.c
  src/util/sockopt.h
  src/util/systemd.c
  src/util/systemd.h
)

# Fallback implementations for disabled features
zephyr_library_sources(
  src/util/apparmor-fallback.c
  src/util/audit-fallback.c
  src/util/selinux-fallback.c
)

# DBus core - D-Bus核心功能
zephyr_library_sources(
  src/dbus/message.c
  src/dbus/message.h
  src/dbus/protocol.c
  src/dbus/protocol.h
  src/dbus/socket.c
  src/dbus/socket.h
  src/dbus/connection.c
  src/dbus/connection.h
  src/dbus/queue.c
  src/dbus/queue.h
  src/dbus/address.c
  src/dbus/address.h
  src/dbus/sasl.c
  src/dbus/sasl.h
)

# Bus - 总线管理功能
zephyr_library_sources(
  src/bus/bus.c
  src/bus/bus.h
  src/bus/peer.c
  src/bus/peer.h
  src/bus/name.c
  src/bus/name.h
  src/bus/match.c
  src/bus/match.h
  src/bus/reply.c
  src/bus/reply.h
  src/bus/metrics.c
  src/bus/metrics.h
  src/bus/activation.c
  src/bus/activation.h
  src/bus/diag.c
  src/bus/diag.h
  src/bus/driver.c
  src/bus/driver.h
  src/bus/listener.c
  src/bus/listener.h
  src/bus/policy.c
  src/bus/policy.h
)

# Broker - 代理核心功能
zephyr_library_sources(
  src/broker/broker.c
  src/broker/broker.h
  src/broker/controller.c
  src/broker/controller.h
  src/broker/controller-dbus.c
  src/broker/main.c
)