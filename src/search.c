#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "search.h"
#include "board.h"
#include "makemove.h"
#include "eval.h"
#include "move.h"
#include "movepicker.h"
#include "uci.h"
#include "utils.h"

// Check if search is over, or user typed stop
// TODO: handle user stop
static int checkSearchOver(Engine *engine) {
    SearchLimits *limits = &engine->limits;
    if (limits->searchType == LIMIT_INFINITE) {
        return false;
    }

    if (limits->searchType == LIMIT_TIME) {
        if (getTime() >= limits->searchStopTime) {
            engine->searchState = SEARCH_STOPPED;
            return true;
        }
    } else if (limits->searchType == LIMIT_NODES) {
        if (engine->searchStats.nodes >= limits->nodes) {
            engine->searchState = SEARCH_STOPPED;
            return true;
        }
    }

    return false;
}

// Searches until there are no more captures
static int quiesce(Engine *engine, int alpha, int beta) {
    if (engine->searchState == SEARCH_STOPPED) return 0;
    engine->searchStats.nodes++;
    Board *board = &engine->board;

    // Periodically check time up, or user ended search
    if ((engine->searchStats.nodes & 0xFF) == 0) {
        checkSearchOver(engine);
    }
    
    /*
     * During quiescence, since captures are not "forced", we have the choice to
     * not move at all, accepting our current evaluation. This is the "stand pat"
     * score (taken from poker).
     */
    int standPat = evaluate(board);
    
    // Beta cutoff, this position is too good and will be avoided by our enemy.
    if (standPat >= beta)
        return beta;
    
    // Our worst possible score is the current evaluation.
    if (standPat > alpha)
        alpha = standPat;
    
    MovePicker picker;
    initMovePicker(&picker, board);

    Move move;
    while ((move = pickMove(&picker, board)) != NO_MOVE) {

        // Skip non noisy moves
        if (!IsCapture(move)) continue;
        
        // Skip illegal moves
        if (makeMove(board, move) == 0) {
            undoMove(board, move);
            continue;
        }
        
        int score = -quiesce(engine, -beta, -alpha);
        undoMove(board, move);

        if (engine->searchState == SEARCH_STOPPED) return 0;
        
        // Beta cutoff
        if (score >= beta)
            return beta;
        
        // Alpha update
        if (score > alpha)
            alpha = score;
    }
    
    return alpha;
}

// Negamax, with alpha beta pruning
static int search(Engine *engine, PV *pv, int alpha, int beta, int depth, int ply) {
    // Leaf node - drop to quiescence search
    if (depth <= 0) {
        return quiesce(engine, alpha, beta);
    }

    if (engine->searchState == SEARCH_STOPPED) return 0;
    Board *board = &engine->board;
    engine->searchStats.nodes++;

    // Periodically check time up, or user ended search
    if ((engine->searchStats.nodes & 0xFF) == 0) {
        checkSearchOver(engine);
    }

    if (ply != 0) {
        if (isDraw(board)) {
            pv->length = 0;
            return 0;
        }
    }

    PV childPV;
    childPV.length = 0;
    
    // Begin searching all moves in the position
    int bestScore = -INFINITY;
    int movesPlayed = 0;

    MovePicker picker;
    initMovePicker(&picker, board);

    Move move;
    while ((move = pickMove(&picker, board)) != NO_MOVE) {
        
        // Skip illegal moves
        if (makeMove(board, move) == 0) {
            undoMove(board, move);
            continue;
        }
        movesPlayed++;
        
        int score = -search(engine, &childPV, -beta, -alpha, depth - 1, ply + 1);
        undoMove(board, move);

        // Check if search was stopped
        if (engine->searchState == SEARCH_STOPPED) return 0;

        // New best move found
        if (score > bestScore) {
            bestScore = score;
            
            // Update alpha if beaten
            if (score > alpha) {
                alpha = score;

                // Update PV (Bruce Moreland's method)
                pv->length = 1 + childPV.length;
                pv->moves[0] = move;
                memcpy(pv->moves + 1, childPV.moves, sizeof(uint16_t) * childPV.length);

                // Fail high cut-off
                // The move was too good, the opponent will avoid it!
                if (alpha >= beta) {
                    if (movesPlayed == 1) {
                        engine->searchStats.fhf++;
                    }
                    engine->searchStats.fh++;
                    break;
                }

            }
        }
    }

    // If no moves were made, this is either checkmate or stalemate
    if (movesPlayed == 0) {
        bool inCheck = isSquareAttacked(
            board,
            board->side,
            getlsb(board->pieces[KING] & board->colors[board->side])
        );
        pv->length = 0;
        // Checkmate is bad, stalemate is 0
        if (inCheck)
            return -MATE_SCORE + ply;
        else
            return 0;
    }
    
    return bestScore;
}

/* -------------------------------------------------------------------------- */
/*                                  Search IO                                 */
/* -------------------------------------------------------------------------- */

// Prints search information in UCI format
static void printSearchInfo(int depth, int score, Engine *engine) {
    printf("info depth %d ", depth);

    // Print score in centipawns or mate in x moves
    if (abs(score) > MATE_BOUND) {
        // Calculate number of moves to mate
        int movesToMate = (MATE_SCORE - abs(score) + 1) / 2;
        printf("score mate %d ", score > 0 ? movesToMate : -movesToMate);
    } else {
        printf("score cp %d ", score);
    }

    // Print nodes searched
    printf("nodes %d ", engine->searchStats.nodes);

    // Print PV
    printf("pv ");
    PV *pv = &engine->pv;
    for (int i = 0; i < pv->length; i++) {
        printMove(pv->moves[i], false);
        printf(" ");
    }
    printf("\n");
    fflush(stdout);
}

// Iterative deepening:
// https://www.chessprogramming.org/Iterative_Deepening
Move iterativeDeepening(Engine *engine) {
    SearchLimits *limits = &engine->limits;
    Move bestMove = NO_MOVE;

    // Iteratively increase search depth
    for (int depth = 1; depth <= limits->depth; depth++) {
        int score = search(engine, &engine->pv, -INFINITY, INFINITY, depth, 0);
        if (engine->searchState == SEARCH_STOPPED) {
            break;
        }

        bestMove = engine->pv.moves[0];

        printSearchInfo(depth, score, engine);
    }

    return bestMove;
}

// Initialise search data
void initSearch(Engine *engine) {
    // Clear the principal variation
    engine->pv.length = 0;
    memset(engine->pv.moves, NO_MOVE, sizeof(engine->pv.moves));

    // Clear search statistics
    engine->searchStats.nodes = 0;
    engine->searchStats.fh = 0;
    engine->searchStats.fhf = 0;

    // Set engine state
    engine->searchState = SEARCHING;
}
