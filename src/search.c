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
#include "hashtable.h"
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

    // Check for draw
    if (isDraw(board)) {
        return 0;
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
    initMovePicker(&picker, board, NO_MOVE);

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
    Board *board = &engine->board;

    const int rootNode = (ply == 0);
    const int pvNode = (alpha != beta - 1);

    // Leaf node - drop to quiescence search
    if (depth <= 0) {
        return quiesce(engine, alpha, beta);
    }

    // Update nodes searched for UCI reporting
    engine->searchStats.nodes++;

    // Return if search was stopped
    if (engine->searchState == SEARCH_STOPPED) return 0;

    // Probe hash table
    Move hashMove = NO_MOVE;
    int hashDepth, hashScore, hashFlag;
    if (!rootNode) {
        if (hashTableProbe(board->hash, &hashMove, &hashDepth, &hashScore, &hashFlag) == PROBE_SUCCESS) {
            if (hashDepth >= depth && !pvNode) {
                if (hashFlag == BOUND_EXACT ||
                    (hashFlag == BOUND_LOWER && hashScore >= beta) ||
                    (hashFlag == BOUND_UPPER && hashScore <= alpha)) {
                    return hashScore;
                }
            }
        }
    }

    // Periodically check time up, or user ended search
    if ((engine->searchStats.nodes & 0xFF) == 0) {
        checkSearchOver(engine);
    }

    // Check for draws before searching
    if (!rootNode) {
        if (isDraw(board)) {
            pv->length = 0;
            return 0;
        }
    }

    // Begin searching all the moves in the position
    PV childPV;
    childPV.length = 0;
    
    int bestScore = -INFINITY;
    int movesPlayed = 0;

    Move bestMove = NO_MOVE;
    int hashBound = BOUND_UPPER; // Assume upper bound until we find better move

    MovePicker picker;
    initMovePicker(&picker, board, hashMove);

    Move move;
    while ((move = pickMove(&picker, board)) != NO_MOVE) {
        
        // Skip illegal moves
        if (makeMove(board, move) == 0) {
            undoMove(board, move);
            continue;
        }
        movesPlayed++;
        
        /**
         * Principal variation search (PVS), we search the first move with a full
         * window, assuming that the subsequent moves are worse. The other moves
         * can be searched with the faster null window, and if they fail high, we
         * re-search them with a full window.
         * https://www.chessprogramming.org/Principal_Variation_Search
         */
        int score;
        if (movesPlayed == 1) {
            // Full window search for the first move
            score = -search(engine, &childPV, -beta, -alpha, depth - 1, ply + 1);
        } else {
            // Null window search for other moves
            score = -search(engine, &childPV, -alpha - 1, -alpha, depth - 1, ply + 1);

            // If it fails high, we need to re-search with a full window
            if (score > alpha && score < beta) {
                score = -search(engine, &childPV, -beta, -alpha, depth - 1, ply + 1);
            }
        }
        undoMove(board, move);

        // Check if search was stopped
        if (engine->searchState == SEARCH_STOPPED) return 0;

        // New best move found
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            
            // Update alpha if beaten
            if (score > alpha) {
                alpha = score;
                hashBound = BOUND_EXACT;

                if (pvNode) {
                    // Update PV (Bruce Moreland's method)
                    pv->length = 1 + childPV.length;
                    pv->moves[0] = move;
                    memcpy(pv->moves + 1, childPV.moves, sizeof(uint16_t) * childPV.length);
                }

                // Fail high cut-off
                // The move was too good, the opponent will avoid it!
                if (alpha >= beta) {
                    hashBound = BOUND_LOWER;

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


    // Store the results of this search in the hash table
    hashTableStore(board->hash, bestMove, depth, bestScore, hashBound);
    
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

    // Print time taken
    int timeTaken = getTime() - engine->searchStats.searchStartTime;
    printf("time %d ", timeTaken);

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
    engine->searchStats.searchStartTime = getTime();

    // Set engine state
    engine->searchState = SEARCHING;
}
