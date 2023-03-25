#include "bar.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"
#include "x11.h"

void initStatus(BarStatus *status) {
    const unsigned int statusLength =
        (blockCount * (LEN(blocks[0].output) - 1)) +
        (blockCount - 1 + LEADING_DELIMITER) * (LEN(DELIMITER) - 1);

    status->current = (char *)malloc(statusLength);
    status->previous = (char *)malloc(statusLength);
    status->current[0] = '\0';
    status->previous[0] = '\0';
}

void freeStatus(BarStatus *status) {
    free(status->current);
    free(status->previous);
}

int updateStatus(BarStatus *status) {
    strcpy(status->previous, status->current);
    status->current[0] = '\0';

    for (int i = 0; i < blockCount; i++) {
        Block *block = blocks + i;

        if (strlen(block->output)) {
#if LEADING_DELIMITER
            strcat(status->current, DELIMITER);
#else
            if (status->current[0]) strcat(status->current, DELIMITER);
#endif

#if CLICKABLE_BLOCKS
            if (!debugMode && block->signal) {
                char signal[] = {block->signal, '\0'};
                strcat(status->current, signal);
            }
#endif

            strcat(status->current, block->output);
        }
    }
    return strcmp(status->current, status->previous);
}

void writeStatus(BarStatus *status) {
    // Only write out if status has changed
    if (!updateStatus(status)) return;

    if (debugMode) {
        printf("%s\n", status->current);
        return;
    }
    setXRootName(status->current);
}
