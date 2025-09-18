#pragma once

#include "board.h"
#include "movegen.h"
#include "uci.h"

// Search constants
#define INFINITE 100000
#define MATE_SCORE 99000

// If abs(score) > MATE_BOUND then mate was found.
#define MATE_BOUND 98900

// Search limits
#define MAX_DEPTH 100

// Score to return when search is stopped
#define SEARCH_STOPPED_SCORE -200000

// Search parameters
#define LMP_DEPTH 6
#define ASPIRATION_START_SIZE 10
#define ASPIRATION_SCALE_FACTOR 2

// Search functions
Move iterativeDeepening(Engine *engine);
void initSearch(Engine *engine, SearchLimits limits);
void initSearchTables();
