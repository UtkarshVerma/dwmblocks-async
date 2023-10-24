#pragma once

#include <stdbool.h>

#include "block.h"
#include "config.h"
#include "main.h"
#include "util.h"
#include "x11.h"

typedef struct {
#define STATUS_LENGTH                                                        \
    ((LEN(blocks) * (MEMBER_LENGTH(block, output) - 1) + CLICKABLE_BLOCKS) + \
     (LEN(blocks) - 1 + LEADING_DELIMITER + TRAILING_DELIMITER) *            \
         (LEN(DELIMITER) - 1) +                                              \
     1)
    char current[STATUS_LENGTH];
    char previous[STATUS_LENGTH];
#undef STATUS_LENGTH
} status;

status status_new(void);
bool status_update(status* const status);
int status_write(const status* const status, const bool is_debug_mode,
                 x11_connection* const connection);
