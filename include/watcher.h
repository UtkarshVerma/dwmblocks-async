#pragma once

#include <stdbool.h>
#include <sys/poll.h>

#include "main.h"
#include "util.h"

typedef enum {
    SIGNAL_FD = LEN(blocks),
    WATCHER_FD_COUNT,
} watcher_fd_index;

typedef struct pollfd watcher_fd;

typedef struct {
    watcher_fd fds[WATCHER_FD_COUNT];
} watcher;

int watcher_init(watcher *const watcher, const int signal_fd);
int watcher_poll(watcher *const watcher, const int timeout_ms);
bool watcher_fd_is_readable(const watcher_fd *const watcher_fd);
