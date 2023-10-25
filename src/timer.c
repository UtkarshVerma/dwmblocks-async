#include "timer.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "block.h"
#include "util.h"

static unsigned int compute_tick(const block *const blocks,
                                 const unsigned short block_count) {
    unsigned int tick = 0;

    for (unsigned short i = 0; i < block_count; ++i) {
        const block *const block = &blocks[i];
        tick = gcd(block->interval, tick);
    }

    return tick;
}

static unsigned int compute_reset_value(const block *const blocks,
                                        const unsigned short block_count) {
    unsigned int reset_value = 1;

    for (unsigned short i = 0; i < block_count; ++i) {
        const block *const block = &blocks[i];
        reset_value = MAX(block->interval, reset_value);
    }

    return reset_value;
}

timer timer_new(const block *const blocks, const unsigned short block_count) {
    timer timer = {
        .time = 0,
        .tick = compute_tick(blocks, block_count),
        .reset_value = compute_reset_value(blocks, block_count),
    };

    return timer;
}

int timer_arm(timer *const timer) {
    errno = 0;
    (void)alarm(timer->tick);

    if (errno != 0) {
        (void)fprintf(stderr, "error: could not arm timer\n");
        return 1;
    }

    // Wrap `time` to the interval [1, reset_value].
    timer->time = (timer->time + timer->tick) % timer->reset_value + 1;

    return 0;
}
