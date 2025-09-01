#pragma once

#include "bitboards.h"
#include "board.h"

extern U64 PieceKeys[12][64];
extern U64 EpKeys[64];
extern U64 CastleKeys[16];
extern U64 SideKey;

U64 generateHash(Board *board);
void initZobristKeys();
