#pragma once

#include <bits/types/sigset_t.h>

#include "timer.h"

typedef sigset_t signal_set;
typedef int (*signal_refresh_callback)(void);
typedef int (*signal_timer_callback)(timer* const timer);

typedef struct {
    int fd;
    const signal_refresh_callback refresh_callback;
    const signal_timer_callback timer_callback;
} signal_handler;

signal_handler signal_handler_new(
    const signal_refresh_callback refresh_callback,
    const signal_timer_callback timer_callback);
int signal_handler_init(signal_handler* const handler);
int signal_handler_deinit(signal_handler* const handler);
int signal_handler_process(signal_handler* const handler, timer* const timer);
