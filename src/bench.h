#pragma once

#include "perft.h"

/**
 * Runs a full search on many positions and reports total node count and nps.
 * This is mostly for OpenBench compliance but also is useful for checking if
 * search was affected by any changes made.
 */
void bench();
