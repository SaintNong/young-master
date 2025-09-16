#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

#include "perft.h"
#include "bitboards.h"
#include "board.h"
#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "utils.h"

// Simple perft function - counts nodes in position to a certain depth
U64 perft(Board *board, int depth) {
    // leaf node reached
    if (depth == 0)
        return 1ULL;

    // get all pseudolegal moves
    MoveList moves;
    generatePseudoLegalMoves(&moves, board);

    U64 nodes = 0;

    // loop through moves
    for (int i = 0; i < moves.count; i++) {
        Move move = moves.list[i];

        // Skip illegal moves
        if (makeMove(board, move) == 0) {
            undoMove(board, move);
            continue;
        }
        nodes += perft(board, depth - 1);
        undoMove(board, move);
    }
    return nodes;
}

// Perft with a count for each move
// Super helpful for debugging
int perftDivide(Board *board, int depth) {
    printf("Starting perft at depth %d\n", depth);

    U64 nodes = 0;

    // Start move generation
    MoveList moves;
    generatePseudoLegalMoves(&moves, board);

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.list[i];

        // Skip illegals
        if (makeMove(board, moves.list[i]) == 0) {
            undoMove(board, move);
        }

        // Print node count in branch after this move
        U64 nodesThisMove = perft(board, depth - 1);
        printf("%s - %" PRIu64 "\n", moveToString(move), nodesThisMove);
        nodes += nodesThisMove;

        // Undo the move
        undoMove(board, moves.list[i]);
    }

    printf("Total nodes: %" PRIu64 "\n", nodes);

    return nodes;
}

// Benchmarks perft on a position
void perftBench(Board *board, int depth) {
    printf("Starting perft at depth %d\n", depth);

    // Setup timer
    int start = getTime();

    // Do perft
    U64 nodes = perft(board, depth);
    int elapsed = getTime() - start;

    // Print stats
    printf("Nodes found: %" PRIu64 "\n", nodes);

    printf("Time elapsed (ms): %d\n", elapsed);

    // Calculate meganodes per second
    double mnps = (double)nodes / ((double)elapsed / 1000.0) / 1000000.0;
    printf("Meganodes per second: %.2lf\n", mnps);
}

// Runs a perft benchmark on a bunch of positions
void bench() {
    Board board;

    int totalTime = 0;
    U64 totalNodes = 0;
    int totalPassed = 0;

    // Test each position
    printf("========== Perft Benchmark (%d positions) ==========\n", PERFT_POSITION_COUNT);
    for (int i = 0; i < PERFT_POSITION_COUNT; i++) {
        const PerftEntry test = PERFT_TESTS[i];
        parseFen(&board, test.fen);

        // Run perft and time it
        int start = getTime();
        U64 nodes = perft(&board, test.depth);
        int elapsed = getTime() - start;

        totalTime += elapsed;
        totalNodes += nodes;
        
        // Verify node count
        printf("Position %d - ", i + 1);
        if (nodes != test.nodes) {
            printf_fail("FAILED\n");
            printf("FEN: %s\n", test.fen);
            printf("Depth: %d\n", test.depth);
            printf("Expected nodes: %" PRIu64 "\n", test.nodes);
            printf("Actual nodes: %" PRIu64 "\n", nodes);
        } else {
            printf_success("PASS\n");
            totalPassed++;
        }
    }

    // Print summary
    puts("========== Benchmark Results ==========");
    printf("Tests passed: ");
    if (totalPassed == PERFT_POSITION_COUNT)
        printf_success("%d / %d\n", totalPassed, PERFT_POSITION_COUNT);
    else
        printf_fail("%d / %d\n", totalPassed, PERFT_POSITION_COUNT);

    printf(" Total nodes: %" PRIu64 "\n", totalNodes);
    printf("  Time taken: %-6d ms\n", totalTime);
    printf("       Speed: %-6.2f Meganodes/s\n", 
           (double)totalNodes / ((double)totalTime / 1000.0) / 1000000.0);
}
