#include "status.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "block.h"
#include "config.h"
#include "main.h"
#include "util.h"
#include "x11.h"

static bool has_status_changed(const status *const status) {
    return strcmp(status->current, status->previous) != 0;
}

status status_new(void) {
    status status = {
        .current = {[0] = '\0'},
        .previous = {[0] = '\0'},
    };

    return status;
}

bool status_update(status *const status) {
    (void)strcpy(status->previous, status->current);
    status->current[0] = '\0';

    for (unsigned short i = 0; i < LEN(blocks); ++i) {
        const block *const block = &blocks[i];

        if (strlen(block->output) > 0) {
#if LEADING_DELIMITER
            (void)strcat(status->current, DELIMITER);
#else
            if (status->current[0] != '\0') {
                (void)strcat(status->current, DELIMITER);
            }
#endif

#if CLICKABLE_BLOCKS
            if (block->signal > 0) {
                const char signal[] = {(char)block->signal, '\0'};
                (void)strcat(status->current, signal);
            }
#endif

            (void)strcat(status->current, block->output);
        }
    }

#if TRAILING_DELIMITER
    if (status->current[0] != '\0') {
        (void)strcat(status->current, DELIMITER);
    }
#endif

    return has_status_changed(status);
}

int status_write(const status *const status, const bool is_debug_mode,
                 x11_connection *const connection) {
    if (is_debug_mode) {
        (void)printf("%s\n", status->current);
        return 0;
    }

    if (x11_set_root_name(connection, status->current) != 0) {
        return 1;
    }

    return 0;
}
