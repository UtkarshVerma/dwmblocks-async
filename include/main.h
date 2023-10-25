#pragma once

#include <signal.h>

#include "config.h"
#include "util.h"

#define REFRESH_SIGNAL SIGUSR1

// Utilise C's adjacent string concatenation to count the number of blocks.
#define X(...) "."
enum { BLOCK_COUNT = LEN(BLOCKS(X)) - 1 };
#undef X
