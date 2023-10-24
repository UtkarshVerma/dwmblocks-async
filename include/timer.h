#pragma once

#include <signal.h>

#define TIMER_SIGNAL SIGALRM

typedef struct {
    unsigned int time;
    const unsigned int tick;
    const unsigned int reset_value;
} timer;

timer timer_new(void);
int timer_arm(timer *const timer);
