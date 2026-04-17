/* Compatibility header for signalfd on Zephyr */

#ifndef SYS_SIGNALFD_H
#define SYS_SIGNALFD_H

#include <signal.h>
#include <sys/types.h>

/* signalfd constants */
#ifndef SFD_CLOEXEC
#define SFD_CLOEXEC 02000000
#endif

#ifndef SFD_NONBLOCK
#define SFD_NONBLOCK 00004000
#endif

/* signalfd structure */
// struct signalfd_siginfo {
// 	uint32_t ssi_signo;    /* Signal number */
// 	int32_t ssi_errno;     /* Error number (unused) */
// 	int32_t ssi_code;      /* Signal code */
// 	uint32_t ssi_pid;      /* PID of sender */
// 	uint32_t ssi_uid;      /* Real UID of sender */
// 	int32_t ssi_fd;        /* File descriptor (SIGIO) */
// 	uint32_t ssi_tid;      /* Kernel timer ID (POSIX timers) */
// 	uint32_t ssi_band;     /* Band event (SIGIO) */
// 	uint32_t ssi_overrun;  /* POSIX timer overrun count */
// 	uint32_t ssi_trapno;   /* Trap number that caused signal */
// 	int32_t ssi_status;    /* Exit status or signal (SIGCHLD) */
// 	int32_t ssi_int;       /* Integer sent by sigqueue(2) */
// 	void *ssi_ptr;         /* Pointer sent by sigqueue(2) */
// 	uint64_t ssi_utime;    /* User CPU time consumed (SIGCHLD) */
// 	uint64_t ssi_stime;    /* System CPU time consumed (SIGCHLD) */
// 	uint64_t ssi_addr;     /* Address that generated signal (for hardware-generated signals) */
// 	uint8_t ssi_pad[32];   /* Pad size to 128 bytes (allow for future fields) */
// };

/* Function declaration */
int signalfd(int fd, const sigset_t *mask, int flags);

#endif /* SYS_SIGNALFD_H */
