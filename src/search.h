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

/* -------------------------------------------------------------------------- */
/*                          Search Tunable Parameters                         */
/* -------------------------------------------------------------------------- */
// these currently aren't tuned because I unfortunately don't have a server farm

// Late move pruning formula
#define LMP_DEPTH 5
#define LMP_BASE 3
#define LMP_PRODUCT 1

// Late move reduction formula
#define LMR_BASE_REDUCTION 0.25
#define LMR_DIVISOR 2.6

// Reverse futility pruning
#define REVERSE_FUTILITY_DEPTH 6
#define REVERSE_FUTILITY_MARGIN 150

// Null move pruning
#define NULL_MOVE_PRUNING_DEPTH 3
#define NULL_REDUCTION_BASE 4
#define NULL_REDUCTION_DIVISOR 4

// Internal iterative reductions
#define IIR_DEPTH 3

// Aspiration windows
#define ASPIRATION_START_SIZE 10
#define ASPIRATION_SCALE_FACTOR 2

/* -------------------------------------------------------------------------- */
/*                              Search functions                              */
/* -------------------------------------------------------------------------- */

Move iterativeDeepening(Engine *engine);
void initSearch(Engine *engine, SearchLimits limits);
void initSearchTables();
