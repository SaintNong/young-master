#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "movepicker.h"
#include "board.h"
#include "eval.h"
#include "search.h"

/* -------------------------------------------------------------------------- */
/*                                 Move Scorer                                */
/* -------------------------------------------------------------------------- */

// killers[side][ply]
Move killers[2][MAX_PLY];

// history[side][piece][to]
int history[2][NB_PIECES][64];

// https://www.chessprogramming.org/MVV-LVA
// MVV_LVA[victim][attacker]
int MVV_LVA[NB_PIECES][NB_PIECES];

void initMvvLva() {
    const int PIECE_VALUES[NB_PIECES] = {10, 30, 31, 50, 90, 1000};
    
    for (int victim = PAWN; victim <= KING; victim++) {
        for (int attacker = PAWN; attacker <= KING; attacker++) {
            MVV_LVA[victim][attacker] = PIECE_VALUES[victim] * 100
                + (100 - PIECE_VALUES[attacker] / 10);
        }
    }
}

// Returns the score of a move for ordering
int scoreMove(MovePicker *picker, Move move, Board *board) {
    if (IsCapture(move)) {
        // Capture scoring using MVV-LVA
        int victim = board->squares[MoveTo(move)];
        int attacker = board->squares[MoveFrom(move)];

        // If the target square is empty the move is en passant, so the victim is a pawn.
        if (victim == EMPTY)
            victim = PAWN;

        // Validate pieces
        assert(victim >= PAWN && victim <= KING);
        assert(attacker >= PAWN && attacker <= KING);

        return MVV_LVA[victim][attacker] + CAPTURE_BONUS;
    }

    // Killer moves
    if (move == picker->killerOne) return KILLER_ONE_BONUS;
    if (move == picker->killerTwo) return KILLER_TWO_BONUS;

    // History heuristic
    int score = history[board->side][board->squares[MoveFrom(move)]][MoveTo(move)];

    return score;
}

// Clears the killer table
void clearKillerMoves() {
    memset(killers, NO_MOVE, sizeof(killers));
}

/* -------------------------------------------------------------------------- */
/*                                Move History                                */
/* -------------------------------------------------------------------------- */

// Clears the move history table
void clearMoveHistory() {
    memset(history, 0, sizeof(history));
}

// Gets the history of this move
int getMoveHistory(Board *board, Move move) {
    // Extract move information
    int piece = board->squares[MoveFrom(move)];
    int to    = MoveTo(move);

    // Return this move's history
    return history[board->side][piece][to];
}

// Updates history heuristic for a move
void updateMoveHistory(Board *board, Move move, int depth, bool malus) {
    // Only apply move history to quiet moves
    if (IsCapture(move)) return;

    // Find history entry for this move
    int piece = board->squares[MoveFrom(move)];
    int to    = MoveTo(move);
    int *entry = &history[board->side][piece][to];

    // Have negative delta if this is a malus
    int delta  = (malus) ? -depth * depth : depth * depth;

    // Apply delta to entry using exponential decay formula
    *entry += delta - (*entry * abs(delta)) / HISTORY_MAX_VALUE;
    // printf("%d - %s\n", *entry, moveToString(move));

    // Clamp history value to safe range
    if (*entry > HISTORY_MAX_VALUE)  *entry = HISTORY_MAX_VALUE;
    if (*entry < -HISTORY_MAX_VALUE) *entry = -HISTORY_MAX_VALUE;
}


// Sets killer moves at given ply
void updateKillers(int ply, Move move) {
    // Avoid saving same killer twice
    if (killers[0][ply] == move) return;

    // Demote old killer #1 to killer #2
    killers[1][ply] = killers[0][ply];
    killers[0][ply] = move;
}

/* -------------------------------------------------------------------------- */
/*                             Staged Move Picker                             */
/* -------------------------------------------------------------------------- */

// Swaps moves and their scores by index
static void swapMoves(MovePicker *picker, int index1, int index2) {
    // Swap moves
    Move tempMove = picker->moveList.list[index1];
    picker->moveList.list[index1] = picker->moveList.list[index2];
    picker->moveList.list[index2] = tempMove;
    
    // Swap scores
    int tempScore = picker->moveScores[index1];
    picker->moveScores[index1] = picker->moveScores[index2];
    picker->moveScores[index2] = tempScore;
}

// Finds the index of the highest scored move not already picked
static int bestMoveIndex(MovePicker *picker) {
    int bestIndex = picker->currentIndex;
    for (int i = picker->currentIndex + 1; i < picker->moveList.count; i++) {
        if (picker->moveScores[i] > picker->moveScores[bestIndex]) {
            bestIndex = i;
        }
    }
    return bestIndex;
}


/**
 * Move ordering:
 *   - Hash Move
 *   - Captures (Ordered via MVV-LVA)
 *   - Killer One
 *   - Killer Two
 *   - Quiet moves (Ordered via history)
 */

// Initialize the move picker
void initMovePicker(MovePicker *picker, Board *board, Move hashMove) {
    if (hashMove != NO_MOVE)
        picker->stage = STAGE_HASH_MOVE;
    else
        picker->stage = STAGE_GENERATE;

    picker->moveList.count = 0;
    picker->currentIndex = 0;
    picker->hashMove = hashMove;

    // Retrieve killers from table
    picker->killerOne = killers[board->side][0];
    picker->killerTwo = killers[board->side][1];
    
    // Initialize all move scores to 0
    for (int i = 0; i < MAX_LEGAL_MOVES; i++) {
        picker->moveScores[i] = 0;
    }
}

// Picks the next best move
Move pickMove(MovePicker *picker, Board *board) {
    switch (picker->stage) {
        case STAGE_HASH_MOVE:
            picker->stage = STAGE_GENERATE;
            if (picker->hashMove != NO_MOVE) {
                return picker->hashMove;
            }

            // fall through
        case STAGE_GENERATE:
            // Generate moves after hash move
            generatePseudoLegalMoves(&picker->moveList, board);
            
            // Score all moves
            for (int i = 0; i < picker->moveList.count; i++) {
                Move move = picker->moveList.list[i];
                picker->moveScores[i] = scoreMove(picker, move, board);
            }
            
            picker->stage = STAGE_MAIN;

            // fall through
        case STAGE_MAIN:
            // Selects the next best scored move
            while (picker->currentIndex < picker->moveList.count) {
                // Find the next best move in the list
                int bestIndex = bestMoveIndex(picker);
                Move bestMove = picker->moveList.list[bestIndex];

                // Swap the out of the unsorted portion
                swapMoves(picker, bestIndex, picker->currentIndex);
                picker->currentIndex++;

                // Skip hash move
                if (bestMove == picker->hashMove)
                    continue;

                return bestMove;
            }
            
            picker->stage = STAGE_DONE;
            // fall through
        case STAGE_DONE:
            return NO_MOVE;
    }
    
    return NO_MOVE;
}
