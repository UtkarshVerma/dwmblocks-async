#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

typedef struct {
    bool is_debug_mode;
} cli_arguments;

cli_arguments cli_parse_arguments(const char* const argv[], const int argc);

#endif  // CLI_H
