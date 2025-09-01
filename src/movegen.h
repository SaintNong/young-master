#pragma once

#include "board.h"
#include "move.h"

// Move list definition
#define MAX_LEGAL_MOVES 256
typedef struct {
    Move list[MAX_LEGAL_MOVES];
    int count;
} MoveList;

// Mask between king and rook for each castling direction
// Is checked to ensure it's empty before adding castle move
#define CASTLE_MASK_WK 0x60
#define CASTLE_MASK_WQ 0xE
#define CASTLE_MASK_BK 0x6000000000000000
#define CASTLE_MASK_BQ 0xE00000000000000

void generatePseudoLegalMoves(MoveList *moves, Board *board);
void generateLegalMoves(MoveList *moves, Board *board);
void printMoveList(MoveList moves);
