#pragma once

#include "board.h"
#include "movegen.h"
#include "uci.h"

// Search constants
#define INFINITY 100000
#define MATE_SCORE 99000
// If abs(score) > MATE_BOUND then mate was found.
#define MATE_BOUND 98900

// Search limits
#define MAX_DEPTH 100


// Search functions
Move iterativeDeepening(Engine *engine);
void initSearch(Engine *engine);
