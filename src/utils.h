#pragma once

#include "bitboards.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <sys/select.h>
    #include <unistd.h>
    #include <sys/time.h>
#endif

#define LINE_WIDTH 16

// Gets the amount of milliseconds since the unix epoch.
int getTime();

// Generates a random U64 number using XORSHIFT.
U64 randomU64();

// Checks if user typed 'stop' since last poll
int checkUserStop(void);
