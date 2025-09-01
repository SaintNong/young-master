#include <stdio.h>
#include <stdlib.h>

#include "eval.h"
#include "board.h"
#include "bitboards.h"

// Evaluation of the current board state, from the side to move's POV
int evaluate(Board *board) {
    int score = 0;

    // Go through all the pieces
    for (int piece = PAWN; piece <= KING; piece++) {
        U64 pieces = board->pieces[piece];

        // Loop through the pieces of this type
        while (pieces) {
            int square = poplsb(&pieces);
            if (testBit(board->colors[WHITE], square)) {
                // White piece
                score += PSQT[piece][MIRROR_SQ(square)];
                score += MATERIAL_VALUES[piece];
            } else {
                // Black piece
                score -= PSQT[piece][square];
                score -= MATERIAL_VALUES[piece];
            }
        }
    }

    // Bishop pair
    U64 bishops = board->pieces[BISHOP];
    if (popCount(board->colors[WHITE] & bishops) >= 2)
        score += BISHOP_PAIR_VALUE;
    if (popCount(board->colors[BLACK] & bishops) >= 2)
        score -= BISHOP_PAIR_VALUE;

    // Negamax requires score to be from our point of view
    return (board->side == WHITE) ? score : -score;
}

// Debug function to print the current evaluation
void printEvaluation(Board *board) {
    printBoard(board);

    int score = evaluate(board);
    printf("Evaluation: %d (", score);
    if (abs(score) < 25) {
        printf("the position is roughly even");
    } else if (abs(score) < 70) {
        printf(score > 0 ? "White is slightly better" : "Black is slightly better");
    } else if (abs(score) < 150) {
        printf(score > 0 ? "White is better" : "Black is better");
    } else {
        printf(score > 0 ? "White is winning" : "Black is winning");
    }
    
    printf(")\n");
}