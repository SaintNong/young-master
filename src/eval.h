// This is a basic evaluation for now, just so search will work.
// It's made with some self knowledge, and consultation with this wikipedia page:
// https://en.wikipedia.org/wiki/Chess_piece_relative_value
// Eventually, texel tuning, pawn structures, king safety etc are planned.

#pragma once

#include <stdint.h>
#include "board.h"

/**
 * A packed eval score format which combines middlegame and endgame into a single
 * integer. Because of how bitwise operations work, this format lets you add and
 * subtract score values without the midgame and endgame scores affecting each
 * other, but dividing and multiplying will break everything.
 * 
 * I think I got this idea from here, but a lot of top HCE top engines use it.
 * https://github.com/GediminasMasaitis/texel-tuner
 */
#define MakeScore(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define S(midgame, endgame) MakeScore(midgame, endgame)
#define ScoreMG(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define ScoreEG(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))

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

/**
 * Small bonus for the side to move. Generally, in most positions the side to
 * move is able to improve the score by a small amount. Adding a small bonus to
 * the side to move therefore helps to keep the score from fluctuating back and
 * forth. You can read more about it here:
 * 
 * https://www.chessprogramming.org/Odd-Even_Effect#Score_Oscillation
 */
static const int TEMPO = S(10, 10);

// Material values for each piece
static const int MATERIAL_VALUES[NB_PIECES] = {
    S(   90,  105), // Pawn
    S(  290,  290), // Knight
    S(  315,  325), // Bishop
    S(  480,  495), // Rook
    S( 1000,  900), // Queen
    S(    0,    0)  // King
};

// Mobility values for each piece
static const int MOBILITY_VALUES[NB_PIECES] = {
    S(  0,  0), // Pawn
    S(  8,  7), // Knight
    S(  5,  5), // Bishop
    S(  4,  4), // Rook
    S(  0,  4), // Queen
    S( -5,  0)  // King
};

/**
 * Bishop pair value
 * Conventional chess wisdom dictates that having a 'pair' of bishops is desirable,
 * especially in the endgame.
 * https://en.wikipedia.org/wiki/Bishop_(chess)
 */
static const int BISHOP_PAIR_VALUE = S(30, 50);

/**
 * Piece square tables (PSQT) are tables which score how good a piece generally
 * is, when placed on each square.
 * This extremely simple heuristic is a surprisingly (almost scarily) effective
 * substitute for real positional understanding. Especially with tapered eval
 * between the midgame and endgame, you can encode some quite advanced knowledge.
 * Without its piece square tables, this program would go from much stronger than
 * suddenly to completely hopeless.
 * 
 * The piece square tables in my engine are hand-made based on my quite limited
 * knowledge of chess from playing in highschool (1700 FIDE I think?).
 * In all modern engines top engines (which haven't fully ditched HCE for NNUE)
 * the piece square tables are tuned using gradient descent. I'm planning to add
 * this at a later date, once I figure out how to use their texel tuners.
 * 
 * https://www.chessprogramming.org/Piece-Square_Tables
 * TODO: Tune these tables :)
 */

// Piece square tables (from Black POV for easier reading).
static const int PSQT_BASE[NB_PIECES][64] = {
    // PAWN
    {
        S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), 
        S( 50, 180), S( 50, 180), S( 50, 180), S( 50, 180), S( 50, 180), S( 50, 180), S( 50, 180), S( 50, 180), 
        S( 10,  80), S( 20,  70), S( 20,  65), S( 30,  55), S( 30,  55), S( 20,  65), S( 20,  70), S( 10,  80), 
        S(  5,  33), S(  5,  23), S(  5,  20), S( 23,  20), S( 25,  20), S(  5,  20), S(  5,  23), S(  5,  33), 
        S(  2,  15), S( -5,  15), S(  5,  10), S( 17,  10), S( 20,  10), S(  0,  10), S(  0,  15), S(  2,  15), 
        S(  3,   1), S(  0,   0), S(  2,   0), S(  2,   0), S(  3,   0), S( -5,   0), S(  3,   0), S(  4,   1), 
        S(  5,   1), S( 10,   0), S(  0,   0), S( -8,   0), S( -8,   0), S( 13,   0), S( 10,   0), S(  3,   1), 
        S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), 
    },
    // KNIGHT
    {
        S(-30, -30), S(-20, -20), S(-20, -20), S(-20, -20), S(-20, -20), S(-20, -20), S(-20, -20), S(-30, -30), 
        S(-20, -20), S(-10, -10), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(-10, -10), S(-20, -20), 
        S(-20, -20), S(  0,   5), S( 15,  15), S( 25,  25), S( 25,  25), S( 15,  15), S(  0,   5), S(-20, -20), 
        S(-20, -20), S(  5,   5), S( 20,  20), S( 20,  20), S( 20,  20), S( 20,  20), S(  5,   5), S(-20, -20), 
        S(-20, -20), S(  0,   6), S(  7,   7), S( 15,  15), S( 15,  15), S(  8,   7), S(  0,   6), S(-20, -20), 
        S(-20, -20), S(  5,   5), S(  4,   4), S(  4,   4), S(  4,   4), S(  5,   4), S(  4,   5), S(-20, -20), 
        S(-20, -20), S(-10, -10), S(  0, -10), S(  1, -10), S(  1, -10), S(  0, -10), S(-10, -10), S(-20, -20), 
        S(-30, -30), S( -5, -20), S( -3, -20), S( -3, -20), S( -3, -20), S( -3, -20), S( -5, -20), S(-30, -30), 
    },
    // BISHOP
    {
        S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), 
        S( -2,  -2), S( 10,   5), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S( 10,   5), S( -2,  -2), 
        S( -2,  -2), S(  3,   3), S(  5,   5), S( 10,  10), S( 10,  10), S(  5,   5), S(  3,   3), S( -2,  -2), 
        S( -2,  -2), S(  9,   9), S(  6,   6), S( 15,  15), S( 15,  15), S(  6,   6), S(  9,   9), S( -2,  -2), 
        S( -2,  -2), S(  0,   0), S(  9,   9), S( 15,  15), S( 15,  15), S(  9,   9), S(  0,   0), S( -2,  -2), 
        S( -2,  -2), S(  8,   8), S(  5,   5), S(  3,   3), S(  3,   3), S(  8,   8), S(  4,   4), S( -3,  -3), 
        S( -2,  -2), S( 10,   5), S(  1,   1), S(  1,   1), S(  1,   1), S(  1,   1), S( 15,   5), S( -2,  -2), 
        S( -5,  -5), S( -5,  -5), S( -5,  -5), S( -4,  -4), S( -4,  -4), S( -5,  -5), S( -5,  -5), S( -5,  -5), 
    },
    // ROOK
    {
        S(  5,   5), S(  5,   5), S(  5,   5), S(  5,   5), S(  5,   5), S(  5,   5), S(  5,   5), S(  5,   5), 
        S( 10,  10), S( 20,  20), S( 20,  20), S( 20,  30), S( 20,  30), S( 20,  30), S( 20,  20), S( 10,  10), 
        S( -5,  -5), S(  0,   5), S(  3,  10), S(  3,  15), S(  3,  15), S(  3,  10), S(  0,   5), S( -5,  -5), 
        S( -5,  -5), S(  0,   0), S(  3,   3), S(  3,   3), S(  3,   3), S(  3,   3), S(  0,   0), S( -5,  -5), 
        S( -5,  -5), S(  0,   0), S(  3,   3), S(  3,   3), S(  3,   3), S(  3,   3), S(  0,   0), S( -5,  -5), 
        S( -5,  -5), S(  0,   0), S(  3,   3), S(  3,   3), S(  3,   3), S(  3,   3), S(  0,   0), S( -5,  -5), 
        S( -8,  -8), S(  0,   0), S(  0,   3), S(  3,   3), S(  3,   3), S(  0,   3), S(  0,   0), S( -8,  -8), 
        S(-10, -10), S(  3,   3), S(  5,   5), S( 10,   5), S( 10,   5), S(  5,   5), S(  3,   3), S(-10, -10), 
    },
    // QUEEN
    {
        S(-30, -30), S(-25,   0), S(-25,   0), S(-25,   0), S(-25,   0), S(-25,   0), S(-10,   0), S(-30, -30), 
        S(-10, -20), S(-10,   0), S(-20,  10), S(-20,  12), S(-20,  12), S(-20,  10), S(-10,   0), S(-10, -20), 
        S(-10, -20), S(-10,   5), S(-15,  15), S(-15,  25), S(-15,  25), S(-15,  15), S(-10,   5), S(-10, -20), 
        S(-10, -20), S(-10,   5), S(-15,  10), S(-15,  20), S(-15,  20), S(-15,  20), S(-10,   5), S(-10, -20), 
        S( -5, -20), S(-10,   5), S(-10,   5), S(-10,  15), S(-10,  15), S(-10,   5), S(-10,   5), S( -5, -20), 
        S( -5, -20), S(  0,   5), S( -5,   5), S( -5,   5), S( -5,   5), S( -5,   5), S( -5,   5), S( -5, -20), 
        S( -5, -20), S( -5, -10), S( 12, -10), S( -5, -10), S(  0, -10), S(  2, -10), S( -5, -10), S( -5, -20), 
        S(-10, -30), S( -9, -20), S(  0, -20), S( 10, -20), S(-10, -20), S( -9, -20), S(-15, -20), S(-15, -30), 
    },
    // KING
    {
        S(-30, -30), S(-40, -20), S(-40, -20), S(-50, -20), S(-50, -20), S(-40, -20), S(-40, -20), S(-30, -30), 
        S(-30, -10), S(-40,   5), S(-40,   5), S(-50,   5), S(-50,   5), S(-40,   5), S(-40,   5), S(-30, -10), 
        S(-30, -10), S(-40,   5), S(-40,   5), S(-50,   5), S(-50,   5), S(-40,   5), S(-40,   5), S(-30, -10), 
        S(-30, -10), S(-30,   0), S(-30,   5), S(-30,  10), S(-30,  10), S(-30,   5), S(-30,   0), S(-30, -10), 
        S(-20, -10), S(-20,   0), S(-20,   5), S(-20,  10), S(-20,  10), S(-20,   5), S(-20,   0), S(-20, -10), 
        S(-10, -10), S(-20,   0), S(-15,   5), S(-20,   5), S(-20,   5), S(-15,   5), S(-20,   0), S(-10, -10), 
        S( 20, -25), S( 11, -10), S(-10,  -8), S(-10,  -6), S(-10,  -6), S(-10,  -8), S( 12, -10), S( 20, -25), 
        S( 15, -50), S( 25, -40), S(  3, -40), S( -5, -40), S( -5, -40), S(  5, -40), S( 28, -40), S( 18, -50), 
    },
};

// Evaluation public facing functions
int evaluate(Board *board);
void printEvaluation(Board *board);
void initEvaluation();
