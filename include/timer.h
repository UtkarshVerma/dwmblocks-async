#ifndef TIMER_H
#define TIMER_H

#include <signal.h>

#include "block.h"

#define TIMER_SIGNAL SIGALRM

typedef struct {
    unsigned int time;
    const unsigned int tick;
    const unsigned int reset_value;
} timer;

timer timer_new(const block *const blocks, const unsigned short block_count);
int timer_arm(timer *const timer);

#endif  // TIMER_H
