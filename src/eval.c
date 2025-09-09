#include <stdio.h>
#include <stdlib.h>

#include "eval.h"
#include "board.h"
#include "bitboards.h"


/* -------------------------------------------------------------------------- */
/*                             Tapered Evaluation                             */
/* -------------------------------------------------------------------------- */

/**
 * The game phase increments for each piece on the board, weighted by its value.
 * The lower this value, the closer the board is to an endgame. This is used to
 * interpolate the evaluation score between the middlegame and endgame to prevent
 * evaluation discontinuity. This value starts at 24, and lowers to zero by the
 * time a pawn endgame is reached.
 */
int getGamePhase(Board *board) {
    int phase = 0;

    // Loop through all pieces which affect game phase.
    for (int piece = KNIGHT; piece <= QUEEN; piece++) {
        // Increment the phase by how many pieces of that type is on the board.
        phase += popCount(board->pieces[piece]) * GAME_PHASE_INCREMENTS[piece];
    }
    
    // Clamp phase in case someone promotes early.
    if (phase > PHASE_MAX) phase = PHASE_MAX;
    return phase;
}


// Interpolates the middlegame and endgame scores based on the phase.
int taper(int mg, int eg, int phase) {
    return (mg * phase + eg * (PHASE_MAX - phase)) / PHASE_MAX;
}

/* -------------------------------------------------------------------------- */
/*                              Evaluation (main)                             */
/* -------------------------------------------------------------------------- */

// Evaluation of the current board state, from the side to move's POV
int evaluate(Board *board) {

    // Seperate score for middlegame and endgame
    int scoreMG = 0;
    int scoreEG = 0;

    // Go through all the pieces
    for (int piece = PAWN; piece <= KING; piece++) {
        U64 pieces = board->pieces[piece];

        // Loop through the pieces of this type
        while (pieces) {
            int square = poplsb(&pieces);
            if (testBit(board->colors[WHITE], square)) {
                // White
                // Material
                scoreMG += MATERIAL_VALUES_MG[piece];
                scoreEG += MATERIAL_VALUES_EG[piece];

                // Piece square tables
                scoreMG += PSQT_MG[piece][MIRROR_SQ(square)];
                scoreEG += PSQT_EG[piece][MIRROR_SQ(square)];

            } else {
                // Black
                // Material
                scoreMG -= MATERIAL_VALUES_MG[piece];
                scoreEG -= MATERIAL_VALUES_EG[piece];

                // Piece square tables
                scoreMG -= PSQT_MG[piece][square];
                scoreEG -= PSQT_EG[piece][square];
            }
        }
    }

    // Bishop pair
    U64 bishops = board->pieces[BISHOP];
    if (multipleBits(board->colors[WHITE] & bishops)) {
        scoreMG += BISHOP_PAIR_MG;
        scoreEG += BISHOP_PAIR_EG;
    }
    if (multipleBits(board->colors[BLACK] & bishops)) {
        scoreMG -= BISHOP_PAIR_MG;
        scoreEG -= BISHOP_PAIR_EG;
    }

    // Taper the score
    int phase = getGamePhase(board);
    int score = taper(scoreMG, scoreEG, phase);
    // printf("MG: %d EG: %d\n", scoreMG, scoreEG);

    // Tempo bonus
    if (board->side == WHITE) {
        score += TEMPO;
    } else {
        score -= TEMPO;
    }

    // Negamax requires score to be from our point of view
    return (board->side == WHITE) ? score : -score;
}

/* -------------------------------------------------------------------------- */
/*                                  Debugging                                 */
/* -------------------------------------------------------------------------- */

// Debug function to print the current evaluation and phase
void printEvaluation(Board *board) {
    printBoard(board);

    int score = evaluate(board);
    int phase = getGamePhase(board);
    printf("Evaluation: %d\n", score);
    printf("Phase: %d\n", phase);
    
}
