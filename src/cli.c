#include "cli.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

int cli_init(cli_arguments *const args, const char *const argv[],
             const int argc) {
    args->is_debug_mode = false;

    int opt = -1;
    opterr = 0;  // Suppress getopt's built-in invalid opt message
    while ((opt = getopt(argc, (char *const *)argv, "dh")) != -1) {
        switch (opt) {
            case 'd':
                args->is_debug_mode = true;
                break;
            case '?':
                (void)fprintf(stderr, "error: unknown option `-%c'\n", optopt);
                // fall through
            case 'h':
                // fall through
            default:
                (void)fprintf(stderr, "usage: %s [-d]\n", BINARY);
                return 1;
        }
    }

    return 0;
}
