#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "search.h"
#include "board.h"
#include "makemove.h"
#include "eval.h"
#include "move.h"
#include "movepicker.h"
#include "hashtable.h"
#include "uci.h"
#include "utils.h"

// Late move reduction table.
// int reduction = LMR_TABLE[depth][movesPlayed];
int LMR_TABLE[MAX_DEPTH][MAX_LEGAL_MOVES];

// Initialises the Late Move Reduction Table
void initLMRTable() {
    for (int depth = 1; depth < 64; depth++) {
        for (int movesPlayed = 1; movesPlayed < 64; movesPlayed++) {
            // Eyeballed
            int reduction = 0.10 + log(depth) * log(movesPlayed) / 4.0;
            if (reduction < 0)
                reduction = 0;
                
            LMR_TABLE[depth][movesPlayed] = reduction;

            // if (depth <= 14 && movesPlayed <= 35)
            //     printf("D: %d Move: %d R: %d\n", depth, movesPlayed, reduction);
        }
    }
}

// Some random variation to draw scores help avoid blindness to three-fold lines.
int drawScore(U64 nodes) {
    return 3 - (nodes & 0x3);
}

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

// Searches until the position is non tactical to get a more accurate evaluation.
static int quiesce(Engine *engine, int alpha, int beta, int ply) {
    engine->searchStats.nodes++;
    Board *board = &engine->board;

    const int pvNode = (alpha != beta - 1);

    // Search stopping/time management:
    // Periodically check if the search should be stopped.
    if ((engine->searchStats.nodes & 0x3FF) == 0) {
        checkSearchOver(engine);
    }

    // Return up the tree if the search was stopped.
    if (engine->searchState == SEARCH_STOPPED) return 0;

    // Don't search if we're at our max depth to avoid memory corruption.
    if (ply >= MAX_DEPTH - 1) {
        return evaluate(board);
    }

    // Check if this node is a draw before searching further.
    if (isDraw(board)) {
        return drawScore(engine->searchStats.nodes);
    }

    /**
     * Probe our hash table.
     */
    Move hashMove = NO_MOVE;
    int hashDepth, hashScore, hashFlag;
    if (!pvNode) {
        if (hashTableProbe(
            board->hash,
            ply,
            &hashMove,
            &hashDepth,
            &hashScore,
            &hashFlag
        ) == PROBE_SUCCESS) {
            /**
             * We return immediately if the table has an exact score, or a
             * score that produces a cutoff with our lower/upper bound.
             */
            if (hashFlag == BOUND_EXACT ||
                (hashFlag == BOUND_LOWER && hashScore >= beta) ||
                (hashFlag == BOUND_UPPER && hashScore <= alpha)) {
                return hashScore;
            }
        }
    }
    
    /*
     * During quiescence, we are not "forced" to move, i.e. we have the choice to
     * not move at all, accepting the current evaluation. This is the "stand pat"
     * score (taken from poker).
     */
    int standPat = evaluate(board);
    
    // Evaluation pruning. If the evaluation already beats beta, we can stop now.
    if (standPat >= beta)
        return beta;

    // Our lower bound for score is at minimum the current evaluation.
    if (standPat > alpha)
        alpha = standPat;
    
    
    // Begin searching noisy moves in this node.
    int bestScore = standPat;
    int hashBound = BOUND_UPPER;
    Move bestMove = NO_MOVE;
    MovePicker picker;
    initMovePicker(&picker, board, NO_MOVE);

    Move move;
    while ((move = pickMove(&picker, board)) != NO_MOVE) {
        // Skip non noisy moves.
        if (!IsCapture(move)) continue;
        
        // Skip illegal moves.
        if (makeMove(board, move) == 0) {
            undoMove(board, move);
            continue;
        }
        
        // Search the next layer in the tree.
        int score = -quiesce(engine, -beta, -alpha, ply + 1);
        undoMove(board, move);

        // If the search is stopped we want to return as soon as possible.
        if (engine->searchState == SEARCH_STOPPED) return 0;

        // Score beat our best score.
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            
            // Update alpha if beaten.
            if (score > alpha) {
                alpha = score;
                hashBound = BOUND_EXACT;

                // Move was too good, and will be avoided by the opponent.
                if (alpha >= beta) {
                    hashBound = BOUND_LOWER;

                    break;
                }
            }
        }
    }
    
    // Store the results of this search in the hash table
    hashTableStore(board->hash, ply, bestMove, 0, bestScore, hashBound);

    // Propogate the best score we found up the tree.
    return bestScore;
}

// Principal variation search, with fail-soft alpha beta.
static int search(Engine *engine, PV *pv, int alpha, int beta, int depth, int ply) {
    Board *board = &engine->board;
    PV childPV;

    const int rootNode = (ply == 0);
    const int pvNode = (alpha != beta - 1);

    /**
     * When we reach the edge of our search depth, we switch to quiescence search
     * to get a more accurate evaluation with no hanging pieces or tactics.
     */
    if (depth <= 0) {
        return quiesce(engine, alpha, beta, ply + 1);
    }

    // Update nodes searched for UCI reporting
    engine->searchStats.nodes++;

    // Search stopping/time management:
    // Periodically check if the search should be stopped.
    if ((engine->searchStats.nodes & 0x3FF) == 0) {
        checkSearchOver(engine);
    }

    // Return up the tree if the search was stopped.
    if (engine->searchState == SEARCH_STOPPED) return 0;
    
    // Don't search if we're at our max depth to avoid memory corruption.
    if (ply >= MAX_DEPTH - 1) {
        return evaluate(board);
    }

    /**
     * Probe our hash table to see if we've seen this position before.
     * Even if we can't immediately return a score, the hash table still gives
     * us the best move from the past search which we can try first. This is
     * very likely to cause a cutoff, saving us the time of move generation.
     */
    Move hashMove = NO_MOVE;
    int hashDepth, hashScore, hashFlag;
    if (!rootNode) {
        if (hashTableProbe(
            board->hash,
            ply,
            &hashMove,
            &hashDepth,
            &hashScore,
            &hashFlag
        ) == PROBE_SUCCESS) {
            /**
             * The table returned a score of equal or greater accuracy (depth).
             * We don't return immediately in PV nodes to protect the our PV from
             * being cut short.
             */
            if (hashDepth >= depth && !pvNode) {
                /**
                 * We return immediately if the table has an exact score, or a
                 * score that produces a cutoff with our lower/upper bound.
                 */
                if (hashFlag == BOUND_EXACT ||
                    (hashFlag == BOUND_LOWER && hashScore >= beta) ||
                    (hashFlag == BOUND_UPPER && hashScore <= alpha)) {
                    return hashScore;
                }
            }
        }
    }

    // Check if this node is a draw before searching it.
    if (!rootNode) {
        if (isDraw(board)) {
            pv->length = 0;
            return drawScore(engine->searchStats.nodes);
        }
    }

    // Calculate eval and whether we're in check for use later.
    bool inCheck = boardIsInCheck(board);
    int eval = evaluate(board);

    /**
     * Check extension.
     * It's almost certainly a good idea to search deeper when we're in check to
     * solve the problem at hand. Conversely, if we're checking our opponent it's
     * a good idea to search deeper to see if our attack was any good.
     */
    if (inCheck) depth++;

    /**
     * Null move pruning.
     * If our position is so strong that giving our opponent two moves in a row
     * still allows us to stay above beta, then this position will likely end
     * up above beta in a full search, so it's likely safe to prune.
     * https://www.chessprogramming.org/Null_Move_Pruning
     */
    if (
        !pvNode
        && !inCheck
        && eval >= beta
        && depth >= 3
        && !nullMoveIsBad(board) // Don't do null moves when we only have pawns
        && board->history[board->hisPly-1].move != NO_MOVE
    ) {
        // Calculate reduced depth.
        int reduction = 4 + depth / 3;
        int nullDepth = depth - reduction;
        if (nullDepth < 0) nullDepth = 0;

        // Make the null move.
        makeNullMove(board);
        int score = -search(engine, &childPV, -beta, -beta + 1, nullDepth, ply + 1);
        undoNullMove(board);

        // If we are still above beta then we prune this branch.
        if (score >= beta) {
            return beta;
        }
    }

    // Since we couldn't get a fast return, therefore must search the position.
    childPV.length = 0;
    
    int bestScore = -INFINITE;
    int movesPlayed = 0;

    Move bestMove = NO_MOVE;
    int hashBound = BOUND_UPPER;

    // Create a move picker, which picks moves which look better first,
    // shortening our search by creating cutoffs.
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
         * re-search them with a full window to get their real score.
         * https://www.chessprogramming.org/Principal_Variation_Search
         */
        int score;
        if (movesPlayed == 1) {
            // Full window search for the first move
            score = -search(engine, &childPV, -beta, -alpha, depth - 1, ply + 1);
        } else {

            /**
             * Late move reductions (LMR).
             * Under the assumption that our move ordering is quite good, the
             * later moves in the move list are likely to be bad. Hence, we can
             * (probably) safely search them to a lower depth and save the time
             * to search more important variations deeper.
             * https://www.chessprogramming.org/Late_Move_Reductions
             */

            // Compute depth reduction for LMR
            int reducedDepth = depth - 1;
            if (IsQuiet(move) && !inCheck) {
                reducedDepth -= LMR_TABLE[depth][movesPlayed];
                if (reducedDepth < 0)
                    reducedDepth = 0;
            }

            // Null window search for non PV moves.
            score = -search(engine, &childPV, -alpha - 1, -alpha, reducedDepth, ply + 1);

            /**
             * If the move failed high, we need to re-search with a full window
             * and at full depth, since the move beat our best score and we need
             * its precise value.
             */
            if (score > alpha) {
                score = -search(engine, &childPV, -beta, -alpha, depth - 1, ply + 1);
            }
        }
        undoMove(board, move);

        // Checking if search was stopped during move loop to return faster.
        if (engine->searchState == SEARCH_STOPPED) return 0;

        // A new best move was found!
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            
            // Update alpha (the lower bound of score) when it was beaten.
            if (score > alpha) {
                alpha = score;
                hashBound = BOUND_EXACT;

                if (pvNode) {
                    /**
                     * Alpha was beaten in a PV node, meaning our PV needs to be
                     * updated with a different best line.
                     * This uses Bruce Moreland's Method of tracking PV.
                     */
                    pv->length = 1 + childPV.length;
                    pv->moves[0] = move;
                    memcpy(pv->moves + 1, childPV.moves, sizeof(Move) * childPV.length);
                }

                /**
                 * Fail high cutoff.
                 * The move just played was too good, beating our upper bound for
                 * score. This means the rational opponent will avoid it earlier
                 * in the search tree, so to save time we stop searching now.
                 */
                if (alpha >= beta) {
                    hashBound = BOUND_LOWER;

                    /**
                     * Update quiet move ordering heuristics.
                     * We store statistics about the good quiet moves which caused
                     * beta cutoffs, incentivizing our engine to pick them earlier
                     * in similar positions.
                     */
                    if (!IsCapture(move)) {
                        updateMoveHistory(board, board->side, move, depth);
                        updateKillers(ply, move);
                    }

                    // Track move ordering stats
                    if (movesPlayed == 1) {
                        engine->searchStats.fhf++;
                    }
                    engine->searchStats.fh++;
                    break;
                }

            }
        }
    }

    /**
     * If no legal moves were found in this node, it's either checkmate, or
     * stalemate.
     */
    if (movesPlayed == 0) {
        pv->length = 0;
        if (inCheck)
            return -MATE_SCORE + ply;
        else
            return 0;
    }


    // Store the results of this search in the hash table
    hashTableStore(board->hash, ply, bestMove, depth, bestScore, hashBound);
    
    // Propogate the best score we found up the tree.
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
        int score = search(engine, &engine->pv, -INFINITE, INFINITE, depth, 0);
        if (engine->searchState == SEARCH_STOPPED) {
            break;
        }

        bestMove = engine->pv.moves[0];

        printSearchInfo(depth, score, engine);
    }

    return bestMove;
}

// Gets the engine ready to search, with given limits.
void initSearch(Engine *engine, SearchLimits limits) {
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

    // Clear move ordering heuristics
    clearMoveHistory();
    clearKillerMoves();
}
