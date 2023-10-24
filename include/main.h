#pragma once

#include <signal.h>

#include "block.h"
#include "config.h"
#include "util.h"

// Utilise C's adjacent string concatenation to count the number of blocks.
#define X(...) "."
extern block blocks[LEN(BLOCKS(X)) - 1];
#undef X

#define REFRESH_SIGNAL SIGUSR1
