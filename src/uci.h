#pragma once

#include "board.h"
#include "bitboards.h"

#define NAME "Empyrean"
#define VERSION "v0.1"
#define AUTHOR "Ning XZ"

#define INPUT_BUFFER_SIZE 2048

#define MAX_PLY 128

typedef enum {
    LIMIT_DEPTH,
    LIMIT_NODES,
    LIMIT_TIME,
    LIMIT_INFINITE
} SearchType;

typedef enum {
    SEARCH_STOPPED,
    SEARCHING
} SearchState;

// PV line definition
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
    int nodes;                // Number of nodes searched
    int fh;                   // Number of fail highs
    int fhf;                  // Number of fail high first
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
