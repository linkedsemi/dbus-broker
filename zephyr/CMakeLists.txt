# Zephyr integration for dbus-broker library
if(CONFIG_DBUS_BROKER)
    # Add library
    zephyr_library_named(dbus-broker)

    # Include directories
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/include)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/include/sys)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/include/linux)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src/broker)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src/bus)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src/dbus)
    zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src/util)

    # Add subproject include directories to the dbus_broker target
    zephyr_library_include_directories(
        ${ZEPHYR_CURRENT_MODULE_DIR}/subprojects/libcstdaux-1/src
        ${ZEPHYR_CURRENT_MODULE_DIR}/subprojects/libcrbtree-3/src
        ${ZEPHYR_CURRENT_MODULE_DIR}/subprojects/libclist-3/src
        ${ZEPHYR_CURRENT_MODULE_DIR}/subprojects/libcdvar-1/src
        ${ZEPHYR_CURRENT_MODULE_DIR}/subprojects/libcutf8-1/src
        ${ZEPHYR_CURRENT_MODULE_DIR}/subprojects
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/include
    )

    # Add subproject include directories globally
    zephyr_export_include_directories(
        ${ZEPHYR_CURRENT_MODULE_DIR}/subprojects/libcstdaux-1/src
        ${ZEPHYR_CURRENT_MODULE_DIR}/subprojects/libcrbtree-3/src
        ${ZEPHYR_CURRENT_MODULE_DIR}/subprojects/libclist-3/src
        ${ZEPHYR_CURRENT_MODULE_DIR}/subprojects/libcdvar-1/src
        ${ZEPHYR_CURRENT_MODULE_DIR}/subprojects/libcutf8-1/src
        ${ZEPHYR_CURRENT_MODULE_DIR}/src
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/broker
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/include
    )

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
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/error.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/misc.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/string.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/log.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/user.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/fdlist.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/dispatch.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/fs.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/dirwatch.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/nsec.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/proc.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/sampler.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/sockopt.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/systemd.c

        # Fallback implementations (essential)
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/apparmor-fallback.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/audit-fallback.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/util/selinux-fallback.c

        # D-Bus core sources
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/dbus/message.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/dbus/protocol.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/dbus/socket.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/dbus/connection.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/dbus/queue.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/dbus/address.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/dbus/sasl.c

        # Bus management sources
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/bus.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/peer.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/name.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/match.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/reply.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/metrics.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/activation.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/diag.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/driver.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/listener.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/bus/policy.c

        # Broker core sources
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/broker/broker.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/broker/controller.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/broker/controller-dbus.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/src/broker/main.c

        # Zephyr compatibility implementations
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/tool/sys_compat.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/tool/sys_fun.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/tool/flockfile.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/tool/string_compat.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/tool/missing_functions.c

        # Library sources
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/../subprojects/libcrbtree-3/src/c-rbtree.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/../subprojects/libcdvar-1/src/c-dvar.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/../subprojects/libcdvar-1/src/c-dvar-common.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/../subprojects/libcdvar-1/src/c-dvar-reader.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/../subprojects/libcdvar-1/src/c-dvar-writer.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/../subprojects/libcdvar-1/src/c-dvar-type.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/../subprojects/libcutf8-1/src/c-utf8.c
    )

    # Basic compiler warnings
    zephyr_cc_option(-Wall)
    zephyr_cc_option(-Wextra)
    zephyr_cc_option(-Werror=implicit-int)
    zephyr_cc_option(-Wno-unused-but-set-variable)
    zephyr_cc_option(-Wno-pointer-sign)
    zephyr_cc_option(-fno-strict-aliasing)
    zephyr_cc_option(-fno-common)

    # Link basu library for sd-event support
    # Note: This creates a circular dependency, so we use INTERFACE to avoid it
    # target_link_libraries(dbus_broker INTERFACE basu)
    zephyr_library_link_libraries(basu)

endif()
