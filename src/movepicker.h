#pragma once

#include "board.h"
#include "move.h"
#include "movegen.h"

// Later we will add a generate noisy and quiet seperate stage
typedef enum {
    STAGE_HASH_MOVE,
    STAGE_GENERATE,
    STAGE_MAIN,
    STAGE_DONE
} MovePickerStage;

#define CAPTURE_BONUS KILLER_ONE_BONUS + 1
#define KILLER_ONE_BONUS KILLER_TWO_BONUS + 1
#define KILLER_TWO_BONUS 900000

#define HISTORY_MAX_VALUE 16384

// Move picker structure
typedef struct {
    MoveList moveList;
    int moveScores[MAX_LEGAL_MOVES];
    MovePickerStage stage;
    Move hashMove;
    Move killerOne, killerTwo;
    int currentIndex;
} MovePicker;

// Move scoring
void initMvvLva();

void clearMoveHistory();
void clearKillerMoves();

int getMoveHistory(Board *board, Move move);
void updateMoveHistory(Board *board, Move move, int depth, bool malus);
void updateKillers(int ply, Move move);

// Move picker
void initMovePicker(MovePicker *picker, Board *board, Move hashMove);
Move pickMove(MovePicker *picker, Board *board);
