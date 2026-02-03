#pragma once

/*
 * D-Bus Broker Main Entry
 */

#include <c-stdaux.h>
#include <stdlib.h>
#include "util/log.h"

enum {
        _MAIN_SUCCESS,
        MAIN_EXIT,
        MAIN_FAILED,
};

extern int main_arg_controller;

/*
 * Run the broker main loop
 * This function initializes and runs the broker, returning only when
 * the broker exits (MAIN_EXIT) or fails (error code).
 */
int run(Log *log);
