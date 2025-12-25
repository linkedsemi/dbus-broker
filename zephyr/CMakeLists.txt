# Zephyr integration for dbus-broker library

# Add library
zephyr_library_named(dbus_broker)

# Set source directory
set(DBUS_BROKER_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../src)

# Include directories
zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR}/include/sys)
zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR}/include/linux)
zephyr_library_include_directories(${DBUS_BROKER_SOURCE_DIR})
zephyr_library_include_directories(${DBUS_BROKER_SOURCE_DIR}/broker)
zephyr_library_include_directories(${DBUS_BROKER_SOURCE_DIR}/bus)
zephyr_library_include_directories(${DBUS_BROKER_SOURCE_DIR}/dbus)
zephyr_library_include_directories(${DBUS_BROKER_SOURCE_DIR}/util)

# Subproject include directories (essential dependencies)
zephyr_library_include_directories(${DBUS_BROKER_SOURCE_DIR}/../subprojects/libcstdaux-1/src)
zephyr_library_include_directories(${DBUS_BROKER_SOURCE_DIR}/../subprojects/libcrbtree-3/src)
zephyr_library_include_directories(${DBUS_BROKER_SOURCE_DIR}/../subprojects/libclist-3/src)
zephyr_library_include_directories(${DBUS_BROKER_SOURCE_DIR}/../subprojects/libcdvar-1/src)

# Compilation definitions for Zephyr
zephyr_library_compile_definitions(
  __ZEPHYR__=1
  DBUS_BROKER_BUILD_ZEPHYR=1
  _FILE_OFFSET_BITS=64
  _GNU_SOURCE
  _POSIX_C_SOURCE=200809L
  _XOPEN_SOURCE=700
  _DEFAULT_SOURCE
  PACKAGE_VERSION=37
  BINDIR="/usr/bin"
  SYSTEM_CONSOLE_USERS=""
  _TIME_BITS=64
)

# Force include the compatibility layer (with proper string function availability)
zephyr_library_compile_options(-include head.h)

# Core source files from dbus-broker
zephyr_library_sources(
  # Utility sources (core ones that work with Zephyr)
  ${DBUS_BROKER_SOURCE_DIR}/util/error.c
  ${DBUS_BROKER_SOURCE_DIR}/util/misc.c
  ${DBUS_BROKER_SOURCE_DIR}/util/string.c
  ${DBUS_BROKER_SOURCE_DIR}/util/log.c
  ${DBUS_BROKER_SOURCE_DIR}/util/user.c
  ${DBUS_BROKER_SOURCE_DIR}/util/fdlist.c
  ${DBUS_BROKER_SOURCE_DIR}/util/dispatch.c
  ${DBUS_BROKER_SOURCE_DIR}/util/fs.c
  ${DBUS_BROKER_SOURCE_DIR}/util/dirwatch.c
  ${DBUS_BROKER_SOURCE_DIR}/util/nsec.c
  ${DBUS_BROKER_SOURCE_DIR}/util/proc.c
  ${DBUS_BROKER_SOURCE_DIR}/util/sampler.c
  ${DBUS_BROKER_SOURCE_DIR}/util/sockopt.c
  ${DBUS_BROKER_SOURCE_DIR}/util/systemd.c
  
  # Fallback implementations (essential)
  ${DBUS_BROKER_SOURCE_DIR}/util/apparmor-fallback.c
  ${DBUS_BROKER_SOURCE_DIR}/util/audit-fallback.c
  ${DBUS_BROKER_SOURCE_DIR}/util/selinux-fallback.c
  
  # D-Bus core sources
  ${DBUS_BROKER_SOURCE_DIR}/dbus/message.c
  ${DBUS_BROKER_SOURCE_DIR}/dbus/protocol.c
  ${DBUS_BROKER_SOURCE_DIR}/dbus/socket.c
  ${DBUS_BROKER_SOURCE_DIR}/dbus/connection.c
  ${DBUS_BROKER_SOURCE_DIR}/dbus/queue.c
  ${DBUS_BROKER_SOURCE_DIR}/dbus/address.c
  ${DBUS_BROKER_SOURCE_DIR}/dbus/sasl.c
  
  # Bus management sources
  ${DBUS_BROKER_SOURCE_DIR}/bus/bus.c
  ${DBUS_BROKER_SOURCE_DIR}/bus/peer.c
  ${DBUS_BROKER_SOURCE_DIR}/bus/name.c
  ${DBUS_BROKER_SOURCE_DIR}/bus/match.c
  ${DBUS_BROKER_SOURCE_DIR}/bus/reply.c
  ${DBUS_BROKER_SOURCE_DIR}/bus/metrics.c
  ${DBUS_BROKER_SOURCE_DIR}/bus/activation.c
  ${DBUS_BROKER_SOURCE_DIR}/bus/diag.c
  ${DBUS_BROKER_SOURCE_DIR}/bus/driver.c
  ${DBUS_BROKER_SOURCE_DIR}/bus/listener.c
  ${DBUS_BROKER_SOURCE_DIR}/bus/policy.c
  
  # Broker core sources
  ${DBUS_BROKER_SOURCE_DIR}/broker/broker.c
  ${DBUS_BROKER_SOURCE_DIR}/broker/controller.c
  ${DBUS_BROKER_SOURCE_DIR}/broker/main.c
  
  # Zephyr compatibility implementations
  ${CMAKE_CURRENT_LIST_DIR}/tool/sys_compat.c
  ${CMAKE_CURRENT_LIST_DIR}/tool/sys_fun.c
  ${CMAKE_CURRENT_LIST_DIR}/tool/flockfile.c
  ${CMAKE_CURRENT_LIST_DIR}/tool/string_compat.c
  ${CMAKE_CURRENT_LIST_DIR}/tool/missing_functions.c

  # Library sources
  ${CMAKE_CURRENT_LIST_DIR}/../subprojects/libcrbtree-3/src/c-rbtree.c
)

# Basic compiler warnings
zephyr_cc_option(-Wall)
zephyr_cc_option(-Wextra)
zephyr_cc_option(-Werror=implicit-int)
zephyr_cc_option(-Wno-unused-but-set-variable)
zephyr_cc_option(-Wno-pointer-sign)
zephyr_cc_option(-fno-strict-aliasing)
zephyr_cc_option(-fno-common)