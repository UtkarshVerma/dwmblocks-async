#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

#include "block.h"
#include "timer.h"

typedef sigset_t signal_set;
typedef int (*signal_refresh_callback)(block* const blocks,
                                       const unsigned short block_count);
typedef int (*signal_timer_callback)(block* const blocks,
                                     const unsigned short block_code,
                                     timer* const timer);

typedef struct {
    int fd;
    const signal_refresh_callback refresh_callback;
    const signal_timer_callback timer_callback;

    block* const blocks;
    const unsigned short block_count;
} signal_handler;

signal_handler signal_handler_new(
    block* const blocks, const unsigned short block_count,
    const signal_refresh_callback refresh_callback,
    const signal_timer_callback timer_callback);
int signal_handler_init(signal_handler* const handler);
int signal_handler_deinit(signal_handler* const handler);
int signal_handler_process(signal_handler* const handler, timer* const timer);

#endif  // SIGNAL_HANDLER_H
