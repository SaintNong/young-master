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

#define CAPTURE_BONUS 100000

// Move picker structure
typedef struct {
    MoveList moveList;
    int moveScores[MAX_LEGAL_MOVES];
    MovePickerStage stage;
    int currentIndex;
} MovePicker;

// Move scoring
void initMvvLva();

// Move picker
void initMovePicker(MovePicker *picker, Board *board);
Move pickMove(MovePicker *picker, Board *board);
