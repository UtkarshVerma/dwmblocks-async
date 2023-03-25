#pragma once
#include "block.h"
#include "config.h"
#include "util.h"

typedef struct {
    char *current;
    char *previous;
} BarStatus;

extern unsigned short debugMode;

void initStatus(BarStatus *);
void freeStatus(BarStatus *);
void writeStatus(BarStatus *);
