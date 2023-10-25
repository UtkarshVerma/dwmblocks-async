#pragma once

#include <stdbool.h>
#include <sys/poll.h>

#include "block.h"
#include "main.h"

typedef enum {
    SIGNAL_FD = BLOCK_COUNT,
    WATCHER_FD_COUNT,
} watcher_fd_index;

typedef struct pollfd watcher_fd;

typedef struct {
    watcher_fd fds[WATCHER_FD_COUNT];

    const block *const blocks;
    const unsigned short block_count;
} watcher;

watcher watcher_new(const block *const blocks,
                    const unsigned short block_count);
int watcher_init(watcher *const watcher, const int signal_fd);
int watcher_poll(watcher *const watcher, const int timeout_ms);
bool watcher_fd_is_readable(const watcher_fd *const watcher_fd);
