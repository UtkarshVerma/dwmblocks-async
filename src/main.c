#include "main.h"

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

#define BLOCK(cmd, period, sig) \
    {                           \
        .command = cmd,         \
        .interval = period,     \
        .signal = sig,          \
    },

block blocks[] = {BLOCKS(BLOCK)};
#undef BLOCK

static int init_blocks(void) {
    for (unsigned short i = 0; i < LEN(blocks); ++i) {
        block *const block = &blocks[i];
        if (block_init(block) != 0) {
            return 1;
        }
    }

    return 0;
}

static int deinit_blocks(void) {
    for (unsigned short i = 0; i < LEN(blocks); ++i) {
        block *const block = &blocks[i];
        if (block_deinit(block) != 0) {
            return 1;
        }
    }

    return 0;
}

static int execute_blocks(const unsigned int time) {
    for (unsigned short i = 0; i < LEN(blocks); ++i) {
        block *const block = &blocks[i];
        if (!block_must_run(block, time)) {
            continue;
        }

        if (block_execute(&blocks[i], 0) != 0) {
            return 1;
        }
    }

    return 0;
}

static int trigger_event(timer *const timer) {
    if (execute_blocks(timer->time) != 0) {
        return 1;
    }

    if (timer_arm(timer) != 0) {
        return 1;
    }

    return 0;
}

static int refresh_callback(void) {
    if (execute_blocks(0) != 0) {
        return 1;
    }

    return 0;
}

static int event_loop(const bool is_debug_mode,
                      x11_connection *const connection,
                      signal_handler *const signal_handler) {
    timer timer = timer_new();

    // Kickstart the event loop with an initial execution.
    if (trigger_event(&timer) != 0) {
        return 1;
    }

    watcher watcher;
    if (watcher_init(&watcher, signal_handler->fd) != 0) {
        return 1;
    }

    status status = status_new();
    bool is_alive = true;
    while (is_alive) {
        const int event_count = watcher_poll(&watcher, -1);
        if (event_count == -1) {
            return 1;
        }

        int i = 0;
        for (unsigned short j = 0; j < WATCHER_FD_COUNT; ++j) {
            if (i == event_count) {
                break;
            }

            const watcher_fd *const watcher_fd = &watcher.fds[j];
            if (!watcher_fd_is_readable(watcher_fd)) {
                continue;
            }

            ++i;

            if (j == SIGNAL_FD) {
                is_alive = signal_handler_process(signal_handler, &timer) == 0;
                continue;
            }

            block *const block = &blocks[j];
            (void)block_update(block);
        }

        const bool has_status_changed = status_update(&status);
        if (has_status_changed) {
            if (status_write(&status, is_debug_mode, connection) != 0) {
                return 1;
            }
        }
    }

    return 0;
}

int main(const int argc, const char *const argv[]) {
    cli_arguments cli_args;
    if (cli_init(&cli_args, argv, argc) != 0) {
        return 1;
    }

    x11_connection *const connection = x11_connection_open();
    if (connection == NULL) {
        return 1;
    }

    int status = 0;
    if (init_blocks() != 0) {
        status = 1;
        goto x11_close;
    }

    signal_handler signal_handler =
        signal_handler_new(refresh_callback, trigger_event);
    if (signal_handler_init(&signal_handler) != 0) {
        status = 1;
        goto deinit_blocks;
    }

    if (event_loop(cli_args.is_debug_mode, connection, &signal_handler) != 0) {
        status = 1;
    }

    if (signal_handler_deinit(&signal_handler) != 0) {
        status = 1;
    }

deinit_blocks:
    if (deinit_blocks() != 0) {
        status = 1;
    }

x11_close:
    x11_connection_close(connection);

    return status;
}
