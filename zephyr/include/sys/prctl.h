/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Compatibility header for sys/prctl.h on Zephyr
 */

#pragma once
#include <zephyr/kernel.h>

/* Process control constants */
#define PR_SET_NAME 15
#define PR_GET_NAME 16
#define PR_CAP_AMBIENT 47
#define PR_CAP_AMBIENT_CLEAR_ALL 2

/* Process control function compatibility */
static inline int prctl(int option, unsigned long arg2, unsigned long arg3,
                       unsigned long arg4, unsigned long arg5)
{
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    
    switch (option) {
        case PR_SET_NAME:
        case PR_GET_NAME:
            /* Zephyr doesn't support process name setting in the same way
             * Return success to avoid breaking applications
             */
            return 0;
        case PR_CAP_AMBIENT:
            /* Zephyr doesn't support capabilities
             * Return success to avoid breaking applications
             */
            return 0;
        default:
            /* Return error for unsupported operations */
            return -1;
    }
}