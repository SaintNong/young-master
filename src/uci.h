#pragma once

#include "board.h"
#include "bitboards.h"

// Engine identifiers
#define NAME "Young Master"
#define VERSION_NUM "v1.0"
#define AUTHOR "Ning XZ"

#ifndef GIT_HASH
#define VERSION VERSION_NUM
#else
#define VERSION VERSION_NUM "-" GIT_HASH
#endif

#ifndef COMPILE_TIME
#define COMPILE_TIME "unknown date"
#endif

// Input handling constants
#define OPTION_BUFFER_SIZE 256
#define FEN_BUFFER_SIZE 256
#define INPUT_BUFFER_SIZE 8192

// UCI option limits
#define HASH_SIZE_MAX 2048
#define HASH_SIZE_DEFAULT 128
#define HASH_SIZE_MIN 1

// The exit condition of the search the engine is doing.
typedef enum {
    LIMIT_DEPTH,
    LIMIT_NODES,
    LIMIT_TIME,
    LIMIT_INFINITE
} SearchType;

// The state the engine search is in
typedef enum {
    SEARCH_STOPPED,
    SEARCHING
} SearchState;

// PV line definition
#define MAX_PLY 128
typedef struct {
    Move moves[MAX_PLY];
    int length;
} PV;

// Limits for the search
typedef struct {
    int depth;                // Depth to search to
    U64 nodes;                // Number of nodes to search
    int searchStopTime;       // Time to stop searching at (in ms since epoch)
    SearchType searchType;    // Type of search from enum above
} SearchLimits;

// Search information collected during a search
typedef struct {
    U64 nodes;                // Number of nodes searched
    int fh;                   // Number of fail highs
    int fhf;                  // Number of fail high first
    int searchStartTime;      // Time when the search started
} SearchInfo;

// The entire state of the engine
typedef struct {
    Board board;
    PV pv;
    SearchInfo searchStats;
    SearchLimits limits;
    SearchState searchState;
} Engine;


void uciLoop();
