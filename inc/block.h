#pragma once
#include "config.h"

typedef struct {
    const char *command;
    const unsigned int interval;
    const unsigned int signal;
    int pipe[2];
    char output[CMDLENGTH * 4 + 1];
} Block;

extern Block blocks[];
extern const unsigned short blockCount;

void execBlock(const Block *, const char *);
void execBlocks(unsigned int);
void updateBlock(Block *);
