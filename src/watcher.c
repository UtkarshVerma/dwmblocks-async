#include "watcher.h"

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>

#include "block.h"
#include "util.h"

static bool watcher_fd_is_readable(const watcher_fd* const watcher_fd) {
    return (watcher_fd->revents & POLLIN) != 0;
}

int watcher_init(watcher* const watcher, const block* const blocks,
                 const unsigned short block_count, const int signal_fd) {
    if (signal_fd == -1) {
        (void)fprintf(
            stderr,
            "error: invalid signal file descriptor passed to watcher\n");
        return 1;
    }

    watcher_fd* const fd = &watcher->fds[SIGNAL_FD];
    fd->fd = signal_fd;
    fd->events = POLLIN;

    for (unsigned short i = 0; i < block_count; ++i) {
        const int block_fd = blocks[i].pipe[READ_END];
        if (block_fd == -1) {
            (void)fprintf(
                stderr,
                "error: invalid block file descriptors passed to watcher\n");
            return 1;
        }

        watcher_fd* const fd = &watcher->fds[i];
        fd->fd = block_fd;
        fd->events = POLLIN;
    }

    return 0;
}

int watcher_poll(watcher* watcher, const int timeout_ms) {
    int event_count = poll(watcher->fds, LEN(watcher->fds), timeout_ms);

    // Don't return non-zero status for signal interruptions.
    if (event_count == -1 && errno != EINTR) {
        (void)fprintf(stderr, "error: watcher could not poll blocks\n");
        return 1;
    }

    watcher->got_signal = watcher_fd_is_readable(&watcher->fds[SIGNAL_FD]);

    watcher->active_block_count = event_count - (int)watcher->got_signal;
    unsigned short i = 0;
    unsigned short j = 0;
    while (i < event_count && j < LEN(watcher->active_blocks)) {
        if (watcher_fd_is_readable(&watcher->fds[j])) {
            watcher->active_blocks[i] = j;
            ++i;
        }

        ++j;
    }

    return 0;
}
