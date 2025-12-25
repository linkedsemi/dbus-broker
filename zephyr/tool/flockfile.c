/* File locking compatibility for dbus-broker on Zephyr */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <errno.h>
#include "dbus_broker_zephyr_compat.h"

/* File locking state structure */
static struct file_lock {
    FILE *file;
    int lock_count;
    struct k_mutex mutex;
} file_locks[16];  /* Support up to 16 locked files */

/* Initialize file locking system */
static int file_locking_initialized = 0;
static K_MUTEX_DEFINE(file_lock_mutex);

static void init_file_locking(void)
{
    if (file_locking_initialized) {
        return;
    }
    
    k_mutex_lock(&file_lock_mutex, K_FOREVER);
    if (!file_locking_initialized) {
        for (int i = 0; i < 16; i++) {
            file_locks[i].file = NULL;
            file_locks[i].lock_count = 0;
            k_mutex_init(&file_locks[i].mutex);
        }
        file_locking_initialized = 1;
    }
    k_mutex_unlock(&file_lock_mutex);
}

/* Find or create a lock entry for the given file */
static struct file_lock *get_file_lock(FILE *file)
{
    init_file_locking();
    
    /* Find existing lock */
    for (int i = 0; i < 16; i++) {
        if (file_locks[i].file == file) {
            return &file_locks[i];
        }
    }
    
    /* Find empty slot */
    for (int i = 0; i < 16; i++) {
        if (file_locks[i].file == NULL) {
            file_locks[i].file = file;
            file_locks[i].lock_count = 0;
            return &file_locks[i];
        }
    }
    
    return NULL;  /* No available slots */
}

/* Release a lock entry */
static void release_file_lock(struct file_lock *lock)
{
    if (lock && lock->lock_count == 0) {
        lock->file = NULL;
    }
}

/* flockfile implementation */
void flockfile(FILE *file)
{
    struct file_lock *lock = get_file_lock(file);
    
    if (lock) {
        k_mutex_lock(&lock->mutex, K_FOREVER);
        lock->lock_count++;
    }
}

/* funlockfile implementation */
void funlockfile(FILE *file)
{
    struct file_lock *lock = get_file_lock(file);
    
    if (lock && lock->lock_count > 0) {
        lock->lock_count--;
        if (lock->lock_count == 0) {
            release_file_lock(lock);
        }
        k_mutex_unlock(&lock->mutex);
    }
}

/* ftrylockfile implementation */
int ftrylockfile(FILE *file)
{
    struct file_lock *lock = get_file_lock(file);
    
    if (!lock) {
        return -1;  /* No available slots */
    }
    
    if (k_mutex_lock(&lock->mutex, K_NO_WAIT) == 0) {
        lock->lock_count++;
        return 0;  /* Success */
    }
    
    return -1;  /* Failed */
}

/* getchar_unlocked implementation */
#ifndef getchar_unlocked
int getchar_unlocked(void)
{
    return getchar();
}
#endif

/* putchar_unlocked implementation */
#ifndef putchar_unlocked
int putchar_unlocked(int c)
{
    return putchar(c);
}
#endif

/* getc_unlocked implementation */
int getc_unlocked(FILE *stream)
{
    return getc(stream);
}

/* putc_unlocked implementation */
int putc_unlocked(int c, FILE *stream)
{
    return putc(c, stream);
}