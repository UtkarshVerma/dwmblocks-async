#ifndef TIMER_H
#define TIMER_H

#include <signal.h>
#include <stdbool.h>

#include "block.h"

#define TIMER_SIGNAL SIGALRM

typedef struct {
    unsigned int time;
    const unsigned int tick;
    const unsigned int reset_value;
} timer;

timer timer_new(const block *const blocks, const unsigned short block_count);
int timer_arm(timer *const timer);
bool timer_must_run_block(const timer *const timer, const block *const block);

#endif  // TIMER_H
