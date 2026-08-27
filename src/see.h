#pragma once

#include <stdbool.h>

#include "board.h"
#include "move.h"

// Material values used by SEE and material-based search pruning.
extern const int SEE_PIECE_VALUES[NB_PIECES + 1];

// Returns whether the material result of move is at least threshold.
bool see(Board *board, Move move, int threshold);
