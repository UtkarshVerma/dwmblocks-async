#include "main.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include "block.h"
#include "cli.h"
#include "config.h"
#include "signal-handler.h"
#include "status.h"
#include "timer.h"
#include "util.h"
#include "watcher.h"
#include "x11.h"

static int init_blocks(block *const blocks, const unsigned short block_count) {
    for (unsigned short i = 0; i < block_count; ++i) {
        block *const block = &blocks[i];
        if (block_init(block) != 0) {
            return 1;
        }
    }

    return 0;
}

static int deinit_blocks(block *const blocks,
                         const unsigned short block_count) {
    for (unsigned short i = 0; i < block_count; ++i) {
        block *const block = &blocks[i];
        if (block_deinit(block) != 0) {
            return 1;
        }
    }

    return 0;
}

static int execute_blocks(block *const blocks,
                          const unsigned short block_count,
                          const timer *const timer) {
    for (unsigned short i = 0; i < block_count; ++i) {
        block *const block = &blocks[i];
        if (!timer_must_run_block(timer, block)) {
            continue;
        }

        if (block_execute(&blocks[i], 0) != 0) {
            return 1;
        }
    }

    return 0;
}

static int trigger_event(block *const blocks, const unsigned short block_count,
                         timer *const timer) {
    if (execute_blocks(blocks, block_count, timer) != 0) {
        return 1;
    }

    if (timer_arm(timer) != 0) {
        return 1;
    }

    return 0;
}

static int refresh_callback(block *const blocks,
                            const unsigned short block_count) {
    if (execute_blocks(blocks, block_count, NULL) != 0) {
        return 1;
    }

    return 0;
}

static int event_loop(block *const blocks, const unsigned short block_count,
                      const bool is_debug_mode,
                      x11_connection *const connection,
                      signal_handler *const signal_handler) {
    timer timer = timer_new(blocks, block_count);

    // Kickstart the event loop with an initial execution.
    if (trigger_event(blocks, block_count, &timer) != 0) {
        return 1;
    }

    watcher watcher;
    if (watcher_init(&watcher, blocks, block_count, signal_handler->fd) != 0) {
        return 1;
    }

    status status = status_new(blocks, block_count);
    bool is_alive = true;
    while (is_alive) {
        if (watcher_poll(&watcher, -1) != 0) {
            return 1;
        }

        if (watcher.got_signal) {
            is_alive = signal_handler_process(signal_handler, &timer) == 0;
        }

        for (unsigned short i = 0; i < watcher.active_block_count; ++i) {
            (void)block_update(&blocks[watcher.active_blocks[i]]);
        }

        const bool has_status_changed = status_update(&status);
        if (has_status_changed &&
            status_write(&status, is_debug_mode, connection) != 0) {
            return 1;
        }
    }

    return 0;
}

int main(const int argc, const char *const argv[]) {
    const cli_arguments cli_args = cli_parse_arguments(argv, argc);
    if (errno != 0) {
        return 1;
    }

    x11_connection *const connection = x11_connection_open();
    if (connection == NULL) {
        return 1;
    }

#define BLOCK(command, interval, signal) block_new(command, interval, signal),
    block blocks[BLOCK_COUNT] = {BLOCKS(BLOCK)};
#undef BLOCK
    const unsigned short block_count = LEN(blocks);

    int status = 0;
    if (init_blocks(blocks, block_count) != 0) {
        status = 1;
        goto x11_close;
    }

    signal_handler signal_handler = signal_handler_new(
        blocks, block_count, refresh_callback, trigger_event);
    if (signal_handler_init(&signal_handler) != 0) {
        status = 1;
        goto deinit_blocks;
    }

    if (event_loop(blocks, block_count, cli_args.is_debug_mode, connection,
                   &signal_handler) != 0) {
        status = 1;
    }

    if (signal_handler_deinit(&signal_handler) != 0) {
        status = 1;
    }

deinit_blocks:
    if (deinit_blocks(blocks, block_count) != 0) {
        status = 1;
    }

x11_close:
    x11_connection_close(connection);

    return status;
}
