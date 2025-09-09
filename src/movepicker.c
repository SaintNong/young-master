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
    int pieceValues[NB_PIECES] = {10, 30, 31, 50, 90, 1000};
    
    for (int victim = PAWN; victim <= KING; victim++) {
        for (int attacker = PAWN; attacker <= KING; attacker++) {
            MVV_LVA[victim][attacker] = pieceValues[victim] * 100
                + (100 - pieceValues[attacker] / 10);
        }
    }
}

// Returns the score of a move for ordering
int scoreMove(MovePicker *picker, Move move, Board *board) {
    if (IsCapture(move)) {
        // Capture scoring using MVV-LVA
        int victim = board->squares[MoveTo(move)];
        int attacker = board->squares[MoveFrom(move)];
        return MVV_LVA[victim][attacker] + CAPTURE_BONUS;
    }

    // Killer moves
    if (move == picker->killerOne) return KILLER_ONE_BONUS;
    if (move == picker->killerTwo) return KILLER_TWO_BONUS;

    // History heuristic
    int score = history[board->side][board->squares[MoveFrom(move)]][MoveTo(move)];

    return score;
}

// Clears the move history table
void clearMoveHistory() {
    memset(history, 0, sizeof(history));
}

// Clears the killer table
void clearKillerMoves() {
    memset(killers, NO_MOVE, sizeof(killers));
}

// Updates history heuristic for a move
void updateMoveHistory(Board *board, int side, Move move, int depth) {
    int piece = board->squares[MoveFrom(move)];
    int to    = MoveTo(move);
   
    // Find history entry for this move
    int *entry = &history[side][piece][to];
    int delta  = depth * depth;

    // Exponential decay for history
    *entry += delta - (*entry * abs(delta)) / HISTORY_DIVISOR;
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

            // Fall through to generate stage
        case STAGE_GENERATE:
            // Generate moves after hash move
            generateLegalMoves(&picker->moveList, board);
            
            // Score all moves
            for (int i = 0; i < picker->moveList.count; i++) {
                Move move = picker->moveList.list[i];
                picker->moveScores[i] = scoreMove(picker, move, board);
            }
            
            picker->stage = STAGE_MAIN;

            // Fall through to main stage
        case STAGE_MAIN:
            if (picker->currentIndex < picker->moveList.count) {
                // Find the best move remaining (selection sort)
                int bestIndex = picker->currentIndex;
                for (int i = picker->currentIndex + 1; i < picker->moveList.count; i++) {
                    if (picker->moveScores[i] > picker->moveScores[bestIndex]) {
                        bestIndex = i;
                    }
                }

                // Swap best move with currentIndex
                if (bestIndex != picker->currentIndex) {
                    int tmpScore = picker->moveScores[picker->currentIndex];
                    picker->moveScores[picker->currentIndex] = picker->moveScores[bestIndex];
                    picker->moveScores[bestIndex] = tmpScore;

                    Move tmp = picker->moveList.list[picker->currentIndex];
                    picker->moveList.list[picker->currentIndex] = picker->moveList.list[bestIndex];
                    picker->moveList.list[bestIndex] = tmp;
                }

                // Don't return the hash move again
                if (picker->moveList.list[picker->currentIndex] == picker->hashMove) {
                    picker->currentIndex++;
                    return pickMove(picker, board);
                }

                // Return next best move
                return picker->moveList.list[picker->currentIndex++];
            } else {
                picker->stage = STAGE_DONE;
                return NO_MOVE;
            }
        
        case STAGE_DONE:
            return NO_MOVE;
    }
    
    return NO_MOVE;
}
