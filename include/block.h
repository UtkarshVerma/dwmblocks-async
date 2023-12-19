#ifndef BLOCK_H
#define BLOCK_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "config.h"
#include "util.h"

typedef struct {
    const char *const command;
    const unsigned int interval;
    const int signal;

    int pipe[PIPE_FD_COUNT];
    char output[MAX_BLOCK_OUTPUT_LENGTH * UTF8_MAX_BYTE_COUNT + 1];
    pid_t fork_pid;
} block;

block block_new(const char *const command, const unsigned int interval,
                const int signal);
int block_init(block *const block);
int block_deinit(block *const block);
int block_execute(block *const block, const uint8_t button);
int block_update(block *const block);

#endif  // BLOCK_H
