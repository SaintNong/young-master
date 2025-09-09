#pragma once

#include <stdint.h>
#include "board.h"

int evaluate(Board *board);
void printEvaluation(Board *board);

// Combined score of middlegame and endgame.
#define MakeScore(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define S(midgame, endgame) MakeScore(midgame, endgame)

/**
 * Table of how much each piece affects the game phase.
 * This is weighted on how much each piece is worth (or how threatening each
 * piece is for starting attacks).
 */
static const int GAME_PHASE_INCREMENTS[NB_PIECES] = {
    0, // Pawn
    1, // Bishop
    1, // Knight
    2, // Rook
    4, // Queen
    0, // King
};

#define PHASE_MAX 24

/* -------------------------------------------------------------------------- */
/*                              Evaluation Terms                              */
/* -------------------------------------------------------------------------- */
// This is a basic evaluation for now, just so search will work.
// It's made with some self knowledge, and consultation with this wikipedia page:
// https://en.wikipedia.org/wiki/Chess_piece_relative_value
// Eventually, texel tuning, pawn structures, king safety etc are planned.

// static const int MATERIAL_VALUES[NB_PIECES] = {
//     S(   85,  100), // Pawn
//     S(  325,  325), // Knight
//     S(  335,  335), // Bishop
//     S(  495,  510), // Rook
//     S( 1000,  930), // Queen
//     S(    0,    0)  // King
// };

// static const int BISHOP_PAIR = S(30, 50);

// Small bonus for the side to move. Generally, in most positions the side to
// move is able to improve the score by a small amount. Adding a small bonus
// therefore helps to keep the score from fluctuating due to the odd-even effect.
static const int TEMPO = 10;

// Piece material values for the middlegame and endgame
static const int MATERIAL_VALUES_MG[NB_PIECES] = {
    85, 325, 335, 495, 1000, 0
};
static const int MATERIAL_VALUES_EG[NB_PIECES] = {
    85, 325, 335, 495, 1000, 0
};

// Bishop pair value
static const int BISHOP_PAIR_MG = 30;
static const int BISHOP_PAIR_EG = 50;

// Piece square tables for middlegame
static const int PSQT_MG[NB_PIECES][64] = {
    // Pawns
    {
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50,
     10, 20, 20, 30, 30, 20, 20, 10,
      5,  5,  5, 23, 25,  5,  5,  5,
      2, -5,  5, 17, 20,  0,  0,  2,
      3,  0,  2,  2,  3, -5,  3,  4,
      5, 10,  0, -8, -8, 13, 10,  3,
      0,  0,  0,  0,  0,  0,  0,  0,
    },
    // Knights
    {
    -30,-20,-20,-20,-20,-20,-20,-30,
    -20,-10,  0,  0,  0,  0,-10,-20,
    -20,  0, 15, 25, 25, 15,  0,-20,
    -20,  5, 20, 20, 20, 20,  5,-20,
    -20,  0,  7, 15, 15,  8,  0,-20,
    -20,  5,  4,  4,  4,  5,  4,-20,
    -20,-10,  0,  1,  1,  0,-10,-20,
    -30, -5, -3, -3, -3, -3, -5,-30,
    },
    // Bishops
    {
    -10,-10,-10,-10,-10,-10,-10,-10,
     -2, 10,  0,  0,  0,  0, 10, -2,
     -2,  3,  5, 10, 10,  5,  3, -2,
     -2,  9,  6, 15, 15,  6,  9, -2,
     -2,  0,  9, 15, 15,  9,  0, -2,
     -2,  8,  5,  3,  3,  8,  4, -3,
     -2, 10,  1,  1,  1,  1, 15, -2,
     -5, -5, -5, -4, -4, -5, -5, -5,
    },
    // Rooks
    {
      5,  5,  5,  5,  5,  5,  5,  5,
     10, 20, 20, 20, 20, 20, 20, 10,
     -5,  0,  3,  3,  3,  3,  0, -5,
     -5,  0,  3,  3,  3,  3,  0, -5,
     -5,  0,  3,  3,  3,  3,  0, -5,
     -5,  0,  3,  3,  3,  3,  0, -5,
     -8,  0,  0,  3,  3,  0,  0, -8,
    -10,  3,  5, 10, 10,  5,  3,-10,
    },
    // Queens
    {
    -30,-25,-25,-25,-25,-25,-10,-30,
    -10,-10,-20,-20,-20,-20,-10,-10,
    -10,-10,-15,-15,-15,-15,-10,-10,
    -10,-10,-15,-15,-15,-15,-10,-10,
     -5,-10,-10,-10,-10,-10,-10, -5,
     -5,  0, -5, -5, -5, -5, -5, -5,
     -5, -5, 12, -5,  0,  2, -5, -5,
    -10, -9,  0, 10,-10, -9,-15,-15,
    },
    // Kings
    {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-30,-30,-30,-30,-30,-30,-30,
    -20,-20,-20,-20,-20,-20,-20,-20,
    -10,-20,-15,-20,-20,-15,-20,-10,
     20, 11,-10,-10,-10,-10, 12, 20,
     15, 25,  3, -5, -5,  5, 28, 18,
    }
};

// Piece square tables for endgame
static const int PSQT_EG[NB_PIECES][64] = {
    // Pawns
    {
       0,   0,   0,   0,   0,   0,   0,   0, 
     120, 120, 120, 120, 120, 120, 120, 120, 
      80,  60,  55,  55,  55,  55,  60,  80, 
      35,  25,  20,  20,  20,  20,  25,  35, 
      15,  15,  10,  10,  10,  10,  15,  15, 
       0,   0,   0,   0,   0,   0,   0,   0, 
       0,   0,   0,   0,   0,   0,   0,   0, 
       0,   0,   0,   0,   0,   0,   0,   0, 
    },
    // Knights
    {
    -30,-20,-20,-20,-20,-20,-20,-30,
    -20,-10,  0,  0,  0,  0,-10,-20,
    -20,  5, 15, 25, 25, 15,  5,-20,
    -20,  5, 20, 20, 20, 20,  5,-20,
    -20,  6,  7, 15, 15,  7,  6,-20,
    -20,  5,  4,  4,  4,  4,  5,-20,
    -20,-10,-10,-10,-10,-10,-10,-20,
    -30,-20,-20,-20,-20,-20,-20,-30,
    },
    // Bishops
    {
    -10,-10,-10,-10,-10,-10,-10,-10,
     -2, 10,  0,  0,  0,  0, 10, -2,
     -2,  3,  5, 10, 10,  5,  3, -2,
     -2,  9,  6, 15, 15,  6,  9, -2,
     -2,  0,  9, 15, 15,  9,  0, -2,
     -2,  8,  5,  3,  3,  8,  4, -3,
     -2, 10,  1,  1,  1,  1, 10, -2,
     -5, -5, -5, -4, -4, -5, -5, -5,
    },
    // Rooks
    {
      5,  5,  5,  5,  5,  5,  5,  5,
     10, 20, 20, 30, 30, 30, 20, 10,
     -5,  5, 10, 15, 15, 10,  5, -5,
     -5,  0,  3,  3,  3,  3,  0, -5,
     -5,  0,  3,  3,  3,  3,  0, -5,
     -5,  0,  3,  3,  3,  3,  0, -5,
     -8,  0,  3,  3,  3,  3,  0, -8,
    -10,  3,  5,  5,  5,  5,  3,-10,
    },
    // Queens
    {
    -30,  0,  0,  0,  0,  0,  0,-30,
    -20,  0, 10, 12, 12, 10,  0,-20,
    -20,  5, 15, 25, 25, 15,  5,-20,
    -20,  5, 10, 20, 20, 20,  5,-20,
    -20,  5,  5, 15, 15,  5,  5,-20,
    -20,  5,  5,  5,  5,  5,  5,-20,
    -20,-10,-10,-10,-10,-10,-10,-20,
    -30,-20,-20,-20,-20,-20,-20,-30,
    },
    // Kings
    {
    -30,-20,-20,-20,-20,-20,-20,-30,
    -10,  5,  5,  5,  5,  5,  5,-10,
    -10,  5,  5,  5,  5,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -25,-10, -8, -6, -6, -8,-10,-25,
    -50,-40,-40,-40,-40,-40,-40,-50,
    }
};

