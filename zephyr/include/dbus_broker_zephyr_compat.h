/* Compatibility layer for dbus-broker on Zephyr */

#ifndef DBUS_BROKER_ZEPHYR_COMPAT_H
#define DBUS_BROKER_ZEPHYR_COMPAT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>

/* Basic type definitions needed VERY EARLY - before network headers */
#ifndef pid_t
typedef int32_t pid_t;
#endif

#ifndef mode_t
typedef uint32_t mode_t;
#endif

/* Type compatibility for gid_t in Zephyr environment */
#ifdef __ZEPHYR__
/* Convert between gid_t and uint32_t for dbus-broker compatibility */
#define GID_TO_UINT32(gid) ((uint32_t)(gid))
#define UINT32_TO_GID(uid) ((gid_t)(uid))
#define GID_ARRAY_TO_UINT32_ARRAY(gids, n_gids) ((const uint32_t *)(gids))
#endif

/* Socket types that might be missing - must be defined before socket headers */
#ifndef sa_family_t
typedef unsigned short sa_family_t;
#endif

#ifndef socklen_t
typedef uint32_t socklen_t;
#endif



/* Function declarations for network operations - net_addr_ntop is provided by Zephyr */

/* Forward declare string functions before including network headers */
#ifdef __ZEPHYR__
extern void *memset(void *ptr, int value, size_t num);
extern void *memcpy(void *dest, const void *src, size_t num);
extern int memcmp(const void *ptr1, const void *ptr2, size_t num);
extern size_t strlen(const char *str);
extern int strcmp(const char *str1, const char *str2);
extern char *strcpy(char *dest, const char *src);
extern char *strchr(const char *str, int c);
#endif

#include <string.h>
#include <stdio.h>

/* Include dbus-broker's c-stdaux to get compiler macros first */
#ifdef __ZEPHYR__
#include <c-stdaux.h>
#endif

/* Only define macros if not already defined by c-stdaux */
#ifndef _c_pure_
#define _c_pure_ __attribute__((pure))
#endif

#ifndef _c_unlikely_
#define _c_unlikely_(x) __builtin_expect(!!(x), 0)
#endif

#ifndef _c_cleanup_
#define _c_cleanup_(func) __attribute__((cleanup(func)))
#endif

#ifndef _c_const_
#define _c_const_ __attribute__((const))
#endif

/* Include Zephyr headers in proper order to get complete definitions */
#ifdef __ZEPHYR__
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/signal.h>

/* Include network headers to get structure definitions */
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>

#endif



#ifndef ssize_t
typedef int32_t ssize_t;
#endif

/* Zephyr compatibility for syslog */
#ifndef LOG_EMERG
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
// #define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */
#endif

/* Zephyr strerror compatibility */
#ifndef strerror
#define strerror(err) "Unknown error"
#endif

/* Manual declarations of memory functions to ensure they're available
 * before including Zephyr headers that might use them
 */
void *memset(void *ptr, int value, size_t num);
void *memcpy(void *dest, const void *src, size_t num);
void *memmove(void *dest, const void *src, size_t num);
int memcmp(const void *ptr1, const void *ptr2, size_t num);
void *memchr(const void *ptr, int value, size_t num);
char *stpcpy(char *dest, const char *src);



/* Program invocation name for compatibility */
extern char *program_invocation_name;
extern char *program_invocation_short_name;

/* Socket and signal function declarations */
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);

/* signalfd function - not provided by Zephyr, so we keep our declaration */
int signalfd(int fd, const sigset_t *mask, int flags);

/* sys/auxv.h compatibility */
#ifndef AT_RANDOM
#define AT_RANDOM 25
#endif

static inline unsigned long getauxval(unsigned long type) {
    static unsigned char random_data[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    
    if (type == AT_RANDOM) {
        return (unsigned long)random_data;
    }
    
    return 0;
}

/* String function declarations for compatibility */
#ifndef __BUILTIN_MEMSET
void *memset(void *ptr, int value, size_t num);
#endif

#ifndef __BUILTIN_MEMCPY  
void *memcpy(void *dest, const void *src, size_t num);
#endif

#ifndef __BUILTIN_MEMCMP
int memcmp(const void *ptr1, const void *ptr2, size_t num);
#endif

#ifndef __BUILTIN_STRLEN
size_t strlen(const char *str);
#endif

#ifndef __BUILTIN_STRCMP
int strcmp(const char *str1, const char *str2);
#endif

#ifndef __BUILTIN_STRNCMP
int strncmp(const char *str1, const char *str2, size_t num);
#endif

#ifndef __BUILTIN_STRNLEN
size_t strnlen(const char *str, size_t maxlen);
#endif

#ifndef __BUILTIN_STRSTR
char *strstr(const char *haystack, const char *needle);
#endif

#ifndef __BUILTIN_STRSPN
size_t strspn(const char *str, const char *delim);
#endif

#ifndef __BUILTIN_STRNDUP
char *strndup(const char *str, size_t len);
#endif

#ifndef __BUILTIN_STRCSPN
size_t strcspn(const char *str, const char *delim);
#endif

#ifndef __BUILTIN_STRCPY
char *strcpy(char *dest, const char *src);
#endif

#ifndef __BUILTIN_STRCHR
char *strchr(const char *str, int c);
#endif

/* Additional string functions */
#ifndef __BUILTIN_STRDUP
char *strdup(const char *str);
#endif

#ifndef __BUILTIN_STRCHRNUL
char *strchrnul(const char *str, int c);
#endif

/* Additional system function declarations */
int setgroups(size_t size, const gid_t *list);
int setgid(gid_t gid);
int setuid(uid_t uid);
int fcntl(int fd, int cmd, ...);
int k_thread_get_id(const void *thread);
extern char *program_invocation_short_name;

/* File functions */
#ifndef __BUILTIN_OPEN
int open(const char *pathname, int flags, ...);
#endif

ssize_t pread(int fd, void *buf, size_t count, off_t offset);

/* Socket functions */
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);

/* c-stdaux compatibility functions */
int c_close(int fd);
void c_closep(int *fdp);

/* Missing constants */
#ifndef F_SETFD
#define F_SETFD 2
#endif
#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif

/* Missing string function */
#ifndef __BUILTIN_STRNCPY
char *strncpy(char *dest, const char *src, size_t n);
#endif

#ifndef __BUILTIN_STRNLEN
size_t strnlen(const char *str, size_t maxlen);
#endif

/* Missing constants */
#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif

#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x100
#endif

#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR 0x200
#endif

#ifndef O_PATH
#define O_PATH 010000000
#endif

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 00400000
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000
#endif

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 02000000
#endif

#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK 00004000
#endif

/* Socket options */
#ifndef SO_PASSCRED
#define SO_PASSCRED 16
#endif

#ifndef SO_PEERCRED
#define SO_PEERCRED 17
#endif

#ifndef SO_PRIORITY
#define SO_PRIORITY 12
#endif

#ifndef SO_RCVBUF
#define SO_RCVBUF 8
#endif

#ifndef SO_SNDBUF
#define SO_SNDBUF 7
#endif

#ifndef SO_TIMESTAMP
#define SO_TIMESTAMP 29
#endif

#ifndef SO_PEERSEC
#define SO_PEERSEC 31
#endif

#ifndef SO_PEERGROUPS
#define SO_PEERGROUPS 30
#endif

/* Message flags */
#ifndef MSG_CMSG_CLOEXEC
#define MSG_CMSG_CLOEXEC 0x40000000
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0x4000
#endif

#ifndef MSG_CTRUNC
#define MSG_CTRUNC 0x0020
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0x0004
#endif

/* Control message types */
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 0x01
#endif

/* File control commands */
#ifndef F_DUPFD_CLOEXEC
#define F_DUPFD_CLOEXEC 1030
#endif

/* mmap flags */
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

/* Protect against Zephyr mman.h redefinition */
// #if !defined(MAP_PRIVATE) || defined(__ZEPHYR__)
// #undef MAP_PRIVATE
// #define MAP_PRIVATE 0x02
// #endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

/* EPoll flags (not supported in Zephyr, define for compatibility) */
#ifndef EPOLLIN
#define EPOLLIN 0x001
#endif

#ifndef EPOLLOUT
#define EPOLLOUT 0x004
#endif

#ifndef EPOLLERR
#define EPOLLERR 0x008
#endif

#ifndef EPOLLHUP
#define EPOLLHUP 0x010
#endif

#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

#ifndef EPOLLET
#define EPOLLET 0x80000000
#endif

#ifndef EPOLL_CLOEXEC
#define EPOLL_CLOEXEC 02000000
#endif

#ifndef EPOLL_CTL_ADD
#define EPOLL_CTL_ADD 1
#endif

#ifndef EPOLL_CTL_DEL
#define EPOLL_CTL_DEL 2
#endif

#ifndef EPOLL_CTL_MOD
#define EPOLL_CTL_MOD 3
#endif

/* inotify flags */
#ifndef IN_CLOEXEC
#define IN_CLOEXEC 02000000
#endif

#ifndef IN_NONBLOCK
#define IN_NONBLOCK 00004000
#endif

/* Socket message flags */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0x4000  /* Don't generate SIGPIPE */
#endif

/* Socket ioctl commands */
#ifndef SIOCOUTQ
#define SIOCOUTQ 0x5411  /* Get output queue size */
#endif

/* Error numbers not defined in Zephyr */
#ifndef ENOTSUP
#define ENOTSUP 524
#endif

#ifndef EPROTO
#define EPROTO 71
#endif

#ifndef ERESTART
#define ERESTART 516
#endif

#ifndef EREMOTEIO
#define EREMOTEIO 121
#endif

#ifndef ETOOMANYREFS
#define ETOOMANYREFS 129  /* Too many references: cannot splice */
#endif

struct passwd *getpwuid(uid_t uid);
int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups);

/* Cred structure */
#ifndef HAVE_STRUCT_UCRED
#define HAVE_STRUCT_UCRED
struct ucred {
    pid_t pid;
    uid_t uid;
    gid_t gid;
};
#endif

/* IOV_MAX definition */
#ifndef IOV_MAX
#define IOV_MAX 1024
#endif

/* dirent compatibility */
#ifdef __ZEPHYR__
struct dirent_compat {
    uint32_t d_ino;
    int32_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[1]; /* Variable length */
};

#define dirent_reclen(de) ((de)->d_reclen ? (de)->d_reclen : \
    (offsetof(struct dirent, d_name) + strlen((de)->d_name) + 1))

/* inotify compatibility for Zephyr (not supported) */
#define IN_ACCESS 0x00000001
#define IN_MODIFY 0x00000002
#define IN_ATTRIB 0x00000004
#define IN_CLOSE_WRITE 0x00000008
#define IN_CLOSE_NOWRITE 0x00000010
#define IN_CLOSE 0x00000018
#define IN_OPEN 0x00000020
#define IN_MOVED_FROM 0x00000040
#define IN_MOVED_TO 0x00000080
#define IN_MOVE 0x000000c0
#define IN_CREATE 0x00000100
#define IN_DELETE 0x00000200
#define IN_DELETE_SELF 0x00000400
#define IN_MOVE_SELF 0x00000800
#define IN_UNMOUNT 0x00002000
#define IN_Q_OVERFLOW 0x00004000
#define IN_IGNORED 0x00008000
#define IN_ONLYDIR 0x01000000
#define IN_DONT_FOLLOW 0x02000000
#define IN_EXCL_UNLINK 0x04000000
#define IN_MASK_ADD 0x20000000
#define IN_ISDIR 0x40000000
#define IN_ONESHOT 0x80000000
#define IN_ALL_EVENTS 0x00000fff

struct inotify_event {
    int wd;
    uint32_t mask;
    uint32_t cookie;
    uint32_t len;
    char name[];
};

int inotify_init(void);
int inotify_init1(int flags);
int inotify_add_watch(int fd, const char *pathname, uint32_t mask);
int inotify_rm_watch(int fd, int wd);

#else
#define dirent_reclen(de) ((de)->d_reclen)
#endif

/* Function declarations */
#ifdef __cplusplus
extern "C" {
#endif



/* Missing function declarations */
ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
int pipe2(int pipefd[2], int flags);

/* mmsghdr structure for sendmmsg - Zephyr doesn't provide this */
struct mmsghdr {
    struct msghdr msg_hdr;  /* Message header */
    uint32_t msg_len;       /* Message length */
};

/* sendmmsg implementation for Zephyr */
static inline int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags) {
    unsigned int i, sent = 0;

    /* Send messages one by one since Zephyr doesn't support sendmmsg natively */
    for (i = 0; i < vlen; ++i) {
        ssize_t r = sendmsg(sockfd, (const struct msghdr *)&msgvec[i].msg_hdr, flags);
        if (r < 0) {
            if (sent > 0) {
                /* Return number of messages sent if some succeeded */
                return sent;
            }
            /* Return error if none succeeded */
            return -1;
        }
        msgvec[i].msg_len = r;
        ++sent;
    }

    return sent;
}

/* epoll event structure */
struct epoll_event {
    uint32_t events;
    union {
        void *ptr;
        int fd;
        uint32_t u32;
        uint64_t u64;
    } data;
};

/* Missing macros */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

/* Forward declarations for c-rbtree types */
typedef struct CRBNode CRBNode;
typedef struct CRBTree CRBTree;

/* Forward declarations for controller */
typedef struct Controller Controller;
typedef struct Message Message;

/* Additional missing function declarations */
int ioctl(int fd, unsigned long request, ...);
int signalfd(int fd, const sigset_t *mask, int flags);
int fchmod(int fd, mode_t mode);

/* c-rbtree function declarations */
void c_rbnode_unlink_stale(CRBNode *n);
CRBNode *c_rbtree_first_postorder(CRBTree *t);
CRBNode *c_rbnode_next_postorder(CRBNode *n);
void c_rbtree_add(CRBTree *t, CRBNode *p, CRBNode **l, CRBNode *n);

/* controller function declarations */
int controller_dbus_dispatch(Controller *controller, Message *message);

#ifdef __cplusplus
}
#endif

#endif /* DBUS_BROKER_ZEPHYR_COMPAT_H */