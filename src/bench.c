#include <stdio.h>
#include <inttypes.h>

#include "board.h"
#include "search.h"
#include "bitboards.h"
#include "bench.h"
#include "uci.h"
#include "utils.h"

// Runs a benchmark test suite on multiple positions
void bench() {
    // Initialise engine just like it is in uci
    Engine engine;
    initEngine(&engine);


    U64 totalNodes = 0;
    int totalTime = 0;
    
    printf("Running benchmark with %d positions...\n", PERFT_POSITION_COUNT);
    
    // Test each position
    for (int i = 0; i < PERFT_POSITION_COUNT; i++) {
        printf("Position %d/%d:\n", i + 1, PERFT_POSITION_COUNT);
        const PerftEntry test = PERFT_TESTS[i];
        
        // Set up the position
        parseFen(&engine.board, test.fen);
        
        // Set up search limits
        SearchLimits limits;
        limits.depth = 14;
        limits.nodes = -1;
        limits.searchType = LIMIT_DEPTH;
        
        // Clear search stats for accurate measurement
        initSearch(&engine, limits);
        
        // Run the search
        int start = getTime();
        initSearch(&engine, limits);
        iterativeDeepening(&engine);
        int elapsed = getTime() - start;
        
        // Accumulate totals
        totalNodes += engine.searchStats.nodes;
        totalTime += elapsed;
    }
    
    // Calculate nodes per second
    double nps = (totalTime > 0) ? (double)totalNodes / ((double)totalTime / 1000.0) : 0;
    
    // Print output report (openbench compatible)
    puts(" ========== Bench Report ========== ");
    printf("Time: %d ms\n", totalTime);
    printf("Nodes searched: %" PRIu64 "\n", totalNodes);
    printf("NPS: %.0f\n", nps);
}