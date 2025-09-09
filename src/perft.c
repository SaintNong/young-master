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

// Perft function - counts nodes in position to a certain depth
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
void bench(Board *board, int depth) {
    printf("Starting perft at depth %d\n", depth);

    // Setup timer
    int startTime = getTime();

    // Do perft
    U64 nodes = perft(board, depth);
    int msElapsed = getTime() - startTime;

    // Print stats
    printf("Nodes found: %" PRIu64 "\n", nodes);

    printf("Time elapsed (ms): %d\n", msElapsed);

    // Calculate meganodes per second
    double nodesPerSecond =
            (double)nodes / ((double)msElapsed / 1000.0) / 1000000.0;
    printf("Meganodes per second: %.2lf\n", nodesPerSecond);
}
