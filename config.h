#ifndef CONFIG_H
#define CONFIG_H

// String used to delimit block outputs in the status.
#define DELIMITER "  "

// Maximum number of Unicode characters that a block can output.
#define MAX_BLOCK_OUTPUT_LENGTH 45

// Control whether blocks are clickable.
#define CLICKABLE_BLOCKS 1

// Control whether a leading delimiter should be prepended to the status.
#define LEADING_DELIMITER 0

// Control whether a trailing delimiter should be appended to the status.
#define TRAILING_DELIMITER 0

// Define blocks for the status feed as X(cmd, interval, signal).
#define BLOCKS(X)         \

    X("dwmb-curr_song",	 0  , 3)  \
    X("dwmb-volume",	 0  , 1)  \
    X("dwmb-memory",	 10 , 2)  \
    X("dwmb-cpu_temp",	 10 , 0)  \
    X("dwmb-date",	 180, 0)  \
    X("dwmb-clock",	 1  , 0)  \
    X("dwmb-battery",	 30 , 0)  

#endif  // CONFIG_H
