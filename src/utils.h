#pragma once

#include "bitboards.h"

// Gets the amount of milliseconds since the unix epoch.
int getTime();

// Generates a random U64 number using XORSHIFT.
U64 randomU64();
