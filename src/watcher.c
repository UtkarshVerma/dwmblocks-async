#include "watcher.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/poll.h>

#include "block.h"
#include "util.h"

watcher watcher_new(const block* const blocks,
                    const unsigned short block_count) {
    watcher watcher = {
        .blocks = blocks,
        .block_count = block_count,
    };

    return watcher;
}

int watcher_init(watcher* const watcher, const int signal_fd) {
    if (signal_fd == -1) {
        fprintf(stderr,
                "error: invalid signal file descriptor passed to watcher\n");
        return 1;
    }

    watcher_fd* const fd = &watcher->fds[SIGNAL_FD];
    fd->fd = signal_fd;
    fd->events = POLLIN;

    for (unsigned short i = 0; i < watcher->block_count; ++i) {
        const int block_fd = watcher->blocks[i].pipe[READ_END];
        if (block_fd == -1) {
            fprintf(
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
    const int event_count = poll(watcher->fds, LEN(watcher->fds), timeout_ms);

    // Don't return non-zero status for signal interruptions.
    if (event_count == -1 && errno != EINTR) {
        (void)fprintf(stderr, "error: watcher could not poll blocks\n");
        return -1;
    }

    return event_count;
}

bool watcher_fd_is_readable(const watcher_fd* const watcher_fd) {
    return (watcher_fd->revents & POLLIN) != 0;
}
