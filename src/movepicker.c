#include <string.h>

#include "movepicker.h"
#include "board.h"

// https://www.chessprogramming.org/MVV-LVA
// MVV-LVA table, indexed by [victim][attacker]
int MVV_LVA[NB_PIECES][NB_PIECES];

void initMvvLva() {
    int pieceValues[NB_PIECES] = {10, 30, 31, 50, 90, 1000};
    
    for (int victim = PAWN; victim <= KING; victim++) {
        for (int attacker = PAWN; attacker <= KING; attacker++) {
            MVV_LVA[victim][attacker] = pieceValues[victim] * 100 + (100 - pieceValues[attacker] / 10);
        }
    }
}

// Initialize the move picker
void initMovePicker(MovePicker *picker, Board *board, Move hashMove) {
    if (hashMove != NO_MOVE)
        picker->stage = STAGE_HASH_MOVE;
    else
        picker->stage = STAGE_GENERATE;

    picker->moveList.count = 0;
    picker->currentIndex = 0;
    picker->hashMove = hashMove;
    
    // Initialize all move scores to 0
    for (int i = 0; i < MAX_LEGAL_MOVES; i++) {
        picker->moveScores[i] = 0;
    }
}

// Returns the score of a move for ordering
int scoreMove(Move move, Board *board) {
    if (IsCapture(move)) {
        int victim = board->squares[MoveTo(move)];
        int attacker = board->squares[MoveFrom(move)];
        return MVV_LVA[victim][attacker] + CAPTURE_BONUS;
    }
    return 0;
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
            // Generate all moves
            generateLegalMoves(&picker->moveList, board);
            
            // Score all moves
            for (int i = 0; i < picker->moveList.count; i++) {
                picker->moveScores[i] = scoreMove(picker->moveList.list[i], board);
            }
            
            picker->stage = STAGE_MAIN;
            // Fall through to main stage

        case STAGE_MAIN:
            if (picker->currentIndex < picker->moveList.count) {
                // Find the best move remaining
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

                    Move tmpMove = picker->moveList.list[picker->currentIndex];
                    picker->moveList.list[picker->currentIndex] = picker->moveList.list[bestIndex];
                    picker->moveList.list[bestIndex] = tmpMove;
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
