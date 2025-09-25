#pragma once

#include "bitboards.h"
#include "uci.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <sys/select.h>
    #include <unistd.h>
    #include <sys/time.h>
#endif

// Width of line of user input for 'stop' while searching
#define LINE_WIDTH 16

/* -------------------------------------------------------------------------- */
/*                               Utility Macros                               */
/* -------------------------------------------------------------------------- */

/**
 * ANSI Colour Codes from:
 * https://gist.github.com/RabaDabaDoba/145049536f815903c79944599c6f952a
 */
#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"

#define CRESET "\e[0m"

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

/* -------------------------------------------------------------------------- */
/*                              Utility functions                             */
/* -------------------------------------------------------------------------- */

int clamp(int x, int low, int high);

// Gets the amount of milliseconds since the unix epoch.
int getTime();

// Generates a random U64 number using XORSHIFT.
U64 randomU64();

// Checks if user typed 'stop' since last poll
int checkUserStop();

// ANSI colour printing helper functions
void printf_success(const char *format, ...);
void printf_fail(const char *format, ...);
