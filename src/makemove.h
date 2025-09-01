#pragma once

#include "board.h"
#include "move.h"

int makeMove(Board *board, Move move);
void undoMove(Board *board, Move move);
void makeNullMove(Board *board);
void undoNullMove(Board *board);
