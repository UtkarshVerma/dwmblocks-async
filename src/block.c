#include "block.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static int execLock = 0;

void execBlock(const Block *block, const char *button) {
    unsigned short i = block - blocks;

    // Ensure only one child process exists per block at an instance
    if (execLock & 1 << i) return;
    // Lock execution of block until current instance finishes execution
    execLock |= 1 << i;

    if (fork() == 0) {
        close(block->pipe[0]);
        dup2(block->pipe[1], STDOUT_FILENO);
        close(block->pipe[1]);

        if (button) setenv("BLOCK_BUTTON", button, 1);

        FILE *file = popen(block->command, "r");
        if (!file) {
            printf("\n");
            exit(EXIT_FAILURE);
        }

        // Buffer will hold both '\n' and '\0'
        char buffer[LEN(block->output) + 1];
        if (fgets(buffer, LEN(buffer), file) == NULL) {
            // Send an empty line in case of no output
            printf("\n");
            exit(EXIT_SUCCESS);
        }
        pclose(file);

        // Trim to the max possible UTF-8 capacity
        trimUTF8(buffer, LEN(buffer));

        printf("%s\n", buffer);
        exit(EXIT_SUCCESS);
    }
}

void execBlocks(unsigned int time) {
    for (int i = 0; i < blockCount; i++) {
        const Block *block = blocks + i;
        if (time == 0 ||
            (block->interval != 0 && time % block->interval == 0)) {
            execBlock(block, NULL);
        }
    }
}

void updateBlock(Block *block) {
    char buffer[LEN(block->output)];
    int bytesRead = read(block->pipe[0], buffer, LEN(buffer));

    // String from pipe will always end with '\n'
    buffer[bytesRead - 1] = '\0';

    strcpy(block->output, buffer);

    // Remove execution lock for the current block
    execLock &= ~(1 << (block - blocks));
}
