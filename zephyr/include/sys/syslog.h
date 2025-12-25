#pragma once

/*
 * Compatibility header for sys/syslog.h in Zephyr
 * Provides minimal syslog compatibility for dbus-broker
 */

/* Log levels */
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */

/* Log facilities */
#define LOG_KERN        (0<<3)  /* kernel messages */
#define LOG_USER        (1<<3)  /* random user-level messages */
#define LOG_MAIL        (2<<3)  /* mail system */
#define LOG_DAEMON      (3<<3)  /* system daemons */
#define LOG_AUTH        (4<<3)  /* security/authorization messages */
#define LOG_SYSLOG      (5<<3)  /* messages generated internally by syslogd */
#define LOG_LPR         (6<<3)  /* line printer subsystem */
#define LOG_NEWS        (7<<3)  /* network news subsystem */
#define LOG_UUCP        (8<<3)  /* UUCP subsystem */
#define LOG_CRON        (9<<3)  /* clock daemon */
#define LOG_AUTHPRIV    (10<<3) /* security/authorization messages (private) */
#define LOG_FTP         (11<<3) /* ftp daemon */

/* Minimal stub implementations for Zephyr */
#define LOG_PID    0x01    /* log the process id with each message */
#define LOG_CONS   0x02    /* log on the console if errors in sending */
#define LOG_ODELAY 0x04    /* delay open until first syslog() */
#define LOG_NDELAY 0x08    /* don't delay open */
#define LOG_NOWAIT 0x10    /* don't wait for console forks */
#define LOG_PERROR 0x20    /* log to stderr as well */

/* Stub function declarations */
void openlog(const char *ident, int option, int facility);
void syslog(int priority, const char *format, ...);
void closelog(void);

/* Additional syslog macros */
#define LOG_MAKEPRI(fac, pri) (((fac) << 3) | (pri))
#define LOG_FAC(pri) (((pri) >> 3) & 0xF)
#define LOG_PRI(pri) ((pri) & 0x7)

/* Program name for syslog */
extern char *program_invocation_short_name;