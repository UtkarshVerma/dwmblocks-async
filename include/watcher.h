#ifndef WATCHER_H
#define WATCHER_H

#include <poll.h>
#include <stdbool.h>

#include "block.h"
#include "main.h"

enum watcher_fd_index {
    SIGNAL_FD = BLOCK_COUNT,
    WATCHER_FD_COUNT,
};

typedef struct pollfd watcher_fd;

typedef struct {
    watcher_fd fds[WATCHER_FD_COUNT];
    unsigned short active_blocks[BLOCK_COUNT];
    unsigned short active_block_count;
    bool got_signal;
} watcher;

int watcher_init(watcher *const watcher, const block *const blocks,
                 const unsigned short block_count, const int signal_fd);
int watcher_poll(watcher *const watcher, const int timeout_ms);

#endif  // WATCHER_H
