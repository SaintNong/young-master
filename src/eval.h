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

/* Evaluation constants are generated separately from evaluator logic. */
#include "eval_weights.h"

// Evaluation public facing functions
int evaluate(Board *board);
void printEvaluation(Board *board);
void initEvaluation();
