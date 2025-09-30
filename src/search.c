#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "timeman.h"
#include "search.h"
#include "board.h"
#include "makemove.h"
#include "eval.h"
#include "move.h"
#include "movepicker.h"
#include "hashtable.h"
#include "uci.h"
#include "utils.h"

/* -------------------------------------------------------------------------- */
/*                               Search Helpers                               */
/* -------------------------------------------------------------------------- */

// Late move reduction table.
// int reduction = LMR_TABLE[depth][movesPlayed];
int LMR_TABLE[MAX_DEPTH][MAX_LEGAL_MOVES];
int LMP_TABLE[LMP_DEPTH + 1];

void initSearchTables() {
    // Set all values to zero by default
    memset(LMR_TABLE, 0, sizeof(LMR_TABLE));
    memset(LMP_TABLE, 0, sizeof(LMP_TABLE));
    
    // Initialises the Late Move Reduction Table
    for (int depth = 1; depth < MAX_DEPTH; depth++) {
        for (int movesPlayed = 1; movesPlayed < MAX_LEGAL_MOVES; movesPlayed++) {
            // Eyeballed formula
            int reduction = LMR_BASE_REDUCTION + log(MIN(depth, 64)) * log(MIN(movesPlayed, 64)) / LMR_DIVISOR;
            if (reduction < 0)
                reduction = 0;
                
            LMR_TABLE[depth][movesPlayed] = reduction;

            // if (depth <= 14 && movesPlayed <= 35)
            //     printf("D: %d Move: %d R: %d\n", depth, movesPlayed, reduction);
        }
    }

    // Initialises the Late Pruning Table
    for (int depth = 1; depth <= LMP_DEPTH; depth++) {
        LMP_TABLE[depth] = LMP_BASE + LMP_PRODUCT * depth * depth;
        // printf("%d\n", LMP_TABLE[depth]);
    }
}

// Returns if a score is a mate score
int isMateScore(int score) {
    return (abs(score) > MATE_BOUND);
}

/**
 * Some random variation to let the engine explore positions with many draws
 * more efficiently.
 * Inspired by:
 * https://github.com/official-stockfish/Stockfish/commit/97d2cc9a9c1c4b6ff1b470676fa18c7fc6509886
 */
int drawScore(U64 nodes) {
    return 3 - (nodes & 0x3);
}

// Check if search is over, or user typed stop
static int checkSearchOver(Engine *engine) {
    SearchLimits *limits = &engine->limits;

    if (limits->searchType == LIMIT_INFINITE) {
        // In infinite search, only user stop can end it.
        if (checkUserStop()) {
            engine->searchState = SEARCH_STOPPED;
            return true;
        } else {
            return false;
        }
    }

    if (limits->searchType == LIMIT_TIME) {
        // Check whether we hit the hard bound if we're in a time limited search.
        if (getTime() >= limits->hardBoundTime) {
            engine->searchState = SEARCH_STOPPED;
            return true;
        }
    } else if (limits->searchType == LIMIT_NODES) {
        // Check whether we passed the node limit in node limited search.
        if (engine->searchStats.nodes >= limits->nodes) {
            engine->searchState = SEARCH_STOPPED;
            return true;
        }
    }

    if (checkUserStop()) {
        engine->searchState = SEARCH_STOPPED;
        return true;
    } else {
        return false;
    }
}

/* -------------------------------------------------------------------------- */
/*                              Quiescence Search                             */
/* -------------------------------------------------------------------------- */

// Searches until the position is non tactical to get a more accurate evaluation.
static int quiesce(Engine *engine, int alpha, int beta, int ply) {
    if (engine->searchState == SEARCH_STOPPED) return SEARCH_STOPPED_SCORE;

    // Initialise this node's information.
    Board *board = &engine->board;
    engine->searchStats.nodes++;

    // Classify the node type
    const int pvNode = (alpha != beta - 1);

    // Update seldepth if we've reached a new highest depth
    if (ply + 1 >= engine->searchStats.seldepth)
        engine->searchStats.seldepth = ply + 1;

    /**
     * Time management.
     * Periodically check if the search should be stopped.
     */
    if ((engine->searchStats.nodes & 0xFFF) == 0) {
        checkSearchOver(engine);
    }

    // Return up the tree if the search was stopped.

    // Don't search if we're at our max depth to avoid memory corruption.
    if (ply >= MAX_DEPTH - 1) {
        return evaluate(board);
    }

    // Check if this node is a draw before searching further.
    if (isDraw(board, ply)) {
        return drawScore(engine->searchStats.nodes);
    }

    /**
     * Probe our hash table in qsearch.
     * Here's a debate on whether it's correct to use our hash table in qsearch.
     * From my own testing I found a very slight gain by probing and storing in
     * quiescence, so decided to keep it in, especially since all the top engines
     * are doing it this way now.
     * https://talkchess.com/viewtopic.php?t=47373
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

    // Don't use hash move because it's usually not helpful in qsearch (i think)
    initMovePicker(&picker, board, NO_MOVE);

    Move move;
    while ((move = pickMove(&picker, board)) != NO_MOVE) {
        // Skip non noisy moves.
        if (!IsCapture(move)) break;
        
        // Skip illegal moves.
        if (makeMove(board, move) == 0) {
            undoMove(board, move);
            continue;
        }
        
        // Search the next layer in the tree.
        int score = -quiesce(engine, -beta, -alpha, ply + 1);
        undoMove(board, move);

        // If the search is stopped we want to return as soon as possible.
        if (engine->searchState == SEARCH_STOPPED) return SEARCH_STOPPED_SCORE;

        // Score beat our best score.
        if (score > bestScore) {
            bestScore = score;
            
            // Update alpha if beaten.
            if (score > alpha) {
                alpha = score;
                
                // Alpha being beaten means we have a new best move
                hashBound = BOUND_EXACT;
                bestMove = move;

                // Move was too good, and will be avoided by the opponent.
                if (alpha >= beta) {
                    hashBound = BOUND_LOWER;

                    break;
                }
            }
        }
    }
    
    /**
     * Store the results of this search in the hash table.
     * We store with depth zero so the score is not trusted for cutoffs in the
     * main search tree.
     */
    hashTableStore(board->hash, ply, bestMove, 0, bestScore, hashBound);

    // Propogate the best score we found up the tree.
    return bestScore;
}

/* -------------------------------------------------------------------------- */
/*                                   Search                                   */
/* -------------------------------------------------------------------------- */

// Principal variation search, with fail-soft alpha beta.
static int search(Engine *engine, PV *pv, int alpha, int beta, int depth, int ply, bool cutNode) {
    if (engine->searchState == SEARCH_STOPPED) return SEARCH_STOPPED_SCORE;

    // Initialise this node's information.
    Board *board = &engine->board;
    PV childPV;
    pv->length = 0;

    // Classify the node type
    const int rootNode = (ply == 0);
    const int pvNode = (alpha != beta - 1);

    /**
     * When we reach the edge of our search depth, we switch to quiescence search
     * to take a more accurate evaluation with no tricky hanging pieces or tactics.
     * https://www.chessprogramming.org/Quiescence_Search
     */
    if (depth <= 0) {
        return quiesce(engine, alpha, beta, ply + 1);
    }

    // Update nodes searched for UCI reporting
    engine->searchStats.nodes++;

    /**
     * Time management.
     * Periodically check if the search should be stopped.
     */
    if ((engine->searchStats.nodes & 0xFFF) == 0) {
        checkSearchOver(engine);
    }
    
    // Don't search if we're at our max depth to avoid memory corruption.
    if (ply >= MAX_DEPTH - 1) {
        return evaluate(board);
    }

    /**
     * Look for early exit conditions. We don't take early exit conditions in
     * the root node since that erases our best move.
     */
    if (!rootNode) {
        /**
         * Draw detection. Detects if the board is a draw by 50 move rule,
         * three-fold repetition, or insufficient mating material.
         */
        if (isDraw(board, ply))
            return drawScore(engine->searchStats.nodes);

        /**
         * Mate distance pruning. In positions with a mate we prune lines which
         * cannot possibly be better than this mate. This doesn't add elo but
         * makes the engine much better at mate finding.
         * https://www.chessprogramming.org/Mate_Distance_Pruning
         */
        alpha = MAX(alpha, -MATE_SCORE + ply);
        beta = MIN(beta, MATE_SCORE - ply - 1);

        if (alpha >= beta) return alpha;
    }

    /**
     * Probe our hash table to see if we've seen this position before.
     * Even if we can't immediately return a score, the hash table still gives
     * us the best move from the past search which we can try first. This is
     * very likely to cause a cutoff, saving us the time of move generation.
     * Very useful explanations here:
     * https://www.chessprogramming.org/Transposition_Table
     */
    Move hashMove = NO_MOVE;
    int hashDepth, hashScore, hashFlag;
    if (hashTableProbe(board->hash, ply, &hashMove, &hashDepth, &hashScore, &hashFlag) == PROBE_SUCCESS) {
        /**
         * Do not cutoff at root node since we need a best move. We still grab
         * hash move on root node to speed up move ordering though.
         */
        if (!rootNode) {
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

    // Calculate eval and whether we're in check for use later.
    bool inCheck = boardIsInCheck(board);
    int eval = evaluate(board);

    /**
     * Check extension.
     * It's almost certainly a good idea to search deeper when we're in check to
     * solve the problem at hand. Conversely, if we're checking our opponent it's
     * a good idea to search deeper to see if our attack was any good.
     * https://www.chessprogramming.org/Check_Extensions
     */
    if (inCheck) depth++;

    /**
     * Reverse futility pruning (aka Static Null Move Pruning).
     * (+78.41 elo +/- 19.05)
     * If the static evaluation is in fail-high territory while we're near the
     * horizon, we can assume that this node is likely to fail high and return
     * the evaluation directly.
     * https://www.chessprogramming.org/Reverse_Futility_Pruning
     */
    if (
        !pvNode
        && !inCheck
        && depth <= REVERSE_FUTILITY_DEPTH
    ) {
        int score = eval - REVERSE_FUTILITY_MARGIN * depth;
        if (score >= beta) {
            return score;
        }
    }

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
        && depth >= NULL_MOVE_PRUNING_DEPTH
        && !nullMoveIsBad(board) // Don't do null moves when we only have pawns
        && board->history[board->hisPly-1].move != NO_MOVE
    ) {
        // Calculate reduced depth.
        int reduction = NULL_REDUCTION_BASE + depth / NULL_REDUCTION_DIVISOR;
        int nullDepth = depth - reduction;
        nullDepth = MAX(nullDepth, 0);

        // Make the null move.
        makeNullMove(board);
        int score = -search(engine, &childPV, -beta, -beta + 1, nullDepth, ply + 1, !cutNode);
        undoNullMove(board);

        // If we are still above beta then we prune this branch.
        if (score >= beta) {
            return beta;
        }
    }

    /**
     * Internal Iterative Reductions (IIR). (+13.2 elo)
     * Nodes without a hash move are usually less important, so it's likely safe
     * to reduce them to save time to prioritize more important nodes.
     * https://www.chessprogramming.org/Internal_Iterative_Reductions
     */
    if (!inCheck && depth >= IIR_DEPTH && (pvNode || cutNode) && hashMove == NO_MOVE)
        depth--;

    // Since we couldn't get a fast return, therefore must search the position.
    int bestScore = -INF_SCORE;
    int movesPlayed = 0;
    int quietsPlayed = 0;

    Move bestMove = NO_MOVE;
    int hashBound = BOUND_UPPER;

    // Create a move picker, which picks moves which look better first,
    // shortening our search by creating cutoffs.
    MovePicker picker;
    initMovePicker(&picker, board, hashMove);

    Move move;
    while ((move = pickMove(&picker, board)) != NO_MOVE) {
        /**
         * Late move pruning. (+61.42 elo +/- 17.56)
         * The idea of late move pruning is that at low depths, the quiet moves
         * near the end of the move list are unlikely to succeed at raising alpha
         * and we can probably prune them safely. This can also be thought of as
         * a smooth 'transition' into quiescence search, where we search less
         * and less quiet moves until we eventually hit a point where we search
         * no quiet moves, fully dropping into quiescence search.
         * https://www.talkchess.com/forum/viewtopic.php?t=35955
         */
        if (
            depth <= LMP_DEPTH
            && !pvNode
            && IsQuiet(move)
            && !inCheck
            && quietsPlayed >= LMP_TABLE[depth]
        ) break; // No captures exist after the first quiet in my ordering.
        
        // Skip illegal moves
        if (makeMove(board, move) == 0) {
            undoMove(board, move);
            continue;
        }
        movesPlayed++;
        if (IsQuiet(move)) quietsPlayed++;

        /**
         * At high depths we report the current root move that's being searched.
         */
        if (rootNode && engine->reportCurrMove) {
            printCurrentMove(depth, move, movesPlayed);
        }
        
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
            score = -search(engine, &childPV, -beta, -alpha, depth - 1, ply + 1, false);
        } else {

            /**
             * Late move reductions (LMR).
             * Under the assumption that our move ordering is quite good, the
             * later moves in the move list are likely to be bad. Hence, we can
             * (probably) safely search them to a lower depth and save the time
             * to search more important variations deeper.
             * https://www.chessprogramming.org/Late_Move_Reductions
             */

            // Do not reduce killers because they are important.
            bool isKillerMove = (move == picker.killerOne || move == picker.killerTwo);

            // Compute depth reduction for LMR
            int reducedDepth = depth - 1;
            if (IsQuiet(move) && !inCheck && !isKillerMove) {
                // Basic move-count based reduction
                int reduction = LMR_TABLE[depth][movesPlayed];

                // Apply the reduction then clamp so we don't accidentally extend
                // or go into negative depths
                reducedDepth -= reduction;
                reducedDepth = clamp(reducedDepth, 0, depth - 1);
            }

            // Null window search for non PV moves.
            score = -search(engine, &childPV, -alpha - 1, -alpha, reducedDepth, ply + 1, true);

            /**
             * If the move failed high, we need to re-search with a full window
             * and at full depth, since the move beat our best score and we need
             * its precise value.
             */
            if (score > alpha) {
                score = -search(engine, &childPV, -beta, -alpha, depth - 1, ply + 1, !cutNode);
            }
        }
        undoMove(board, move);

        // Checking if search was stopped during move loop to return faster.
        if (engine->searchState == SEARCH_STOPPED) return SEARCH_STOPPED_SCORE;

        // Update node best score if beaten
        if (score > bestScore) {
            bestScore = score;
            
            // Update alpha (the lower bound of score) when it was beaten.
            if (score > alpha) {
                alpha = score;

                // If alpha is beaten, a new best move was found
                hashBound = BOUND_EXACT;
                bestMove = move;

                /**
                 * If alpha was beaten, our principal variation needs to be updated
                 * with the new best line. (This also implies that we're in a PV
                 * node, since other nodes are searched with zero windows.)
                 * My implementation uses Bruce Moreland's Method of tracking PV.
                 * https://web.archive.org/web/20040620092229/http://www.brucemo.com/compchess/programming/pv.htm
                 * https://www.chessprogramming.org/Principal_Variation
                 */
                pv->length = 1 + childPV.length;
                pv->moves[0] = move;
                memcpy(pv->moves + 1, childPV.moves, sizeof(Move) * childPV.length);

                /**
                 * Fail high cutoff.
                 * The move just played was too good, beating our upper bound for
                 * score. This means the rational opponent will avoid it earlier
                 * in the search tree, so to save time we stop searching now.
                 * https://www.chessprogramming.org/Fail-High
                 */
                if (alpha >= beta) {
                    hashBound = BOUND_LOWER;

                    /**
                     * Update quiet move ordering heuristics.
                     * We store statistics about the good quiet moves which caused
                     * beta cutoffs, incentivizing our engine to pick them earlier
                     * in similar positions.
                     * https://www.chessprogramming.org/Move_Ordering
                     */
                    if (!IsCapture(move)) {
                        // Apply a history bonus to this move.
                        updateMoveHistory(board, board->side, move, depth, false);

                        /**
                         * History Malus. (+39.01 elo +/- 13.66)
                         * Apply a history penalty to the moves before this one,
                         * to incentivize the program to pick this move earlier.
                         * https://www.chessprogramming.org/History_Heuristic#History_Maluses
                         */
                        for (int i = 0; i < movesPlayed - 1; i++) {
                            Move malusMove = picker.moveList.list[i];
                            updateMoveHistory(board, board->side, malusMove, depth, true);
                        }

                        updateKillers(ply, move);
                    }
                    break;
                }
            }
        }
    }

    /**
     * If no legal moves were found in this node, it's either checkmate or
     * stalemate.
     */
    if (movesPlayed == 0) {
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

// Prints the current root move being considered
void printCurrentMove(int depth, Move move, int movesPlayed) {
    printf("info depth %d currmove %s currmovenumber %d\n",
        depth,
        moveToString(move),
        movesPlayed
    );
}

// Prints search information in UCI format
static void printSearchInfo(int depth, int score, Engine *engine) {
    printf("info depth %d ", depth);

    // Print max selective depth reached (if different from depth)
    if (engine->searchStats.seldepth != depth)
        printf("seldepth %d ", engine->searchStats.seldepth);

    // Print score in centipawns or mate in x moves
    if (isMateScore(score)) {
        // Calculate number of moves to mate
        int movesToMate = (MATE_SCORE - abs(score) + 1) / 2;
        printf("score mate %d ", score > 0 ? movesToMate : -movesToMate);
    } else {
        printf("score cp %d ", score);
    }

    // Print nodes searched
    printf("nodes %" PRIu64 " ", engine->searchStats.nodes);

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

/**
 * Aspiration windows. (+59.55 elo +/- 16.91)
 * Aspiration windows are a search improvement that assumes that the score for the
 * current iteration will be roughly the same as the score for the previous one.
 * Therefore, we can search with a reduced window around the last iteration's
 * score, only expanding the window if the score falls outside the guessed bound.
 * https://www.chessprogramming.org/Aspiration_Windows
 */
int aspirationWindow(Engine *engine, PV *pv, int depth, int lastScore) {
    // Reset this ply's search stats
    engine->searchStats.seldepth = 0;

    // Set margin start sizes
    int betaMargin = ASPIRATION_START_SIZE;
    int alphaMargin = ASPIRATION_START_SIZE;

    // Do not use aspiration windows on low depth or mate scores
    if (depth >= 6 && !isMateScore(lastScore)) {
        // Search until we either get a score, or we fall out of our margin
        while (alphaMargin <= 500 && betaMargin <= 500) {
            // Set alpha and beta around the window
            int alpha = lastScore - alphaMargin;
            int beta = lastScore + betaMargin;

            // Search with this window
            int score = search(engine, pv, alpha, beta, depth, 0, false);
            
            // Break out quickly if we're out of time
            if (getTime() > engine->limits.hardBoundTime)
                return SEARCH_STOPPED_SCORE;
            
            // Return our score if it was in the window
            if (score > alpha && score < beta)
                return score;

            // Widen the window
            if (score <= alpha)
                alphaMargin *= ASPIRATION_SCALE_FACTOR;
            if (score >= beta)
                betaMargin *= ASPIRATION_SCALE_FACTOR;
        }
    }

    // Full window search if we fall out of [-500, 500]
    return search(engine, pv, -INF_SCORE, INF_SCORE, depth, 0, false);
}

// Iterative deepening loop
// https://www.chessprogramming.org/Iterative_Deepening
Move iterativeDeepening(Engine *engine) {
    SearchLimits *limits = &engine->limits;
    PV currentPV = {0};

    // Our current best estimate of the root score
    // The aspiration window after a certain depth is centered around this score.
    int rootScore = 0;

    // Iteratively increase search depth
    for (int depth = 1; depth <= limits->depth; depth++) {
        // Stop before the next iteration if we reach our time soft bound.
        // Also stop if we've hit one of our limits (nodes, time, manual stop).
        if (timeSoftBoundReached(&engine->limits) || engine->searchState == SEARCH_STOPPED)
            break;

        // Run a search at this depth
        int score = aspirationWindow(engine, &currentPV, depth, rootScore);

        // Update the root score
        if (score != SEARCH_STOPPED_SCORE)
            rootScore = score;

        // Update our PV if a full line can be recovered from this search.
        if (currentPV.length > 0)
            engine->pv = currentPV;

        // Print this iteration's info string
        printSearchInfo(depth, rootScore, engine);

        // Turn on currmove reporting after some time has passed
        if (getTime() > engine->limits.searchStartTime + REPORT_CURRMOVE_AFTER)
            engine->reportCurrMove = true;
    }
    engine->searchState = SEARCH_STOPPED;

    // Take best move from hash
    Move hashMove = probeHashMove(engine->board.hash);
    if (hashMove != NO_MOVE)
        return hashMove;
    
    // Fallback to PV
    return engine->pv.moves[0];
}

// Gets the engine ready to search, with given limits.
void initSearch(Engine *engine, SearchLimits limits) {
    // Clear the principal variation
    engine->pv.length = 0;
    memset(engine->pv.moves, NO_MOVE, sizeof(engine->pv.moves));

    // Clear search statistics
    engine->searchStats.nodes = 0;
    engine->searchStats.searchStartTime = getTime();
    engine->searchStats.seldepth = 0;

    // Set engine state and search limits
    engine->searchState = SEARCHING;
    engine->limits = limits;
    engine->reportCurrMove = false;

    // Clear move ordering heuristics
    clearMoveHistory();
    clearKillerMoves();
}
