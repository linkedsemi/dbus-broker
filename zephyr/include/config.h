/* Configuration definitions for dbus-broker on Zephyr */

#ifndef CONFIG_H
#define CONFIG_H

/* System configuration */
#define CONFIG_APPARMOR 0
#define CONFIG_AUDIT 0
#define CONFIG_SELINUX 0
#define CONFIG_LAUNCHER 0

/* Zephyr-specific configurations */
#define CONFIG_ZEPHYR 1
#define CONFIG_NO_SYSTEMD 1
#define CONFIG_NO_LIBSYSTEMD 1

/* Disable features not supported in Zephyr */
#define HAVE_ACCEPT4 0
#define HAVE_MEMFD_CREATE 0
#define HAVE_NAME_TO_HANDLE_AT 0
#define HAVE_PIPE2 0

#endif /* CONFIG_H */