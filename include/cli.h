#pragma once

#include <stdbool.h>

typedef struct {
    bool is_debug_mode;
} cli_arguments;

int cli_init(cli_arguments* const args, const char* const argv[],
             const int argc);
