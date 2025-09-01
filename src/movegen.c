
#include <stdio.h>

#include "movegen.h"
#include "bitboards.h"
#include "board.h"
#include "magicmoves.h"
#include "makemove.h"
#include "move.h"

// Adds normal moves, and captures
static inline void addNormalMoves(MoveList *moves, int fromSq, U64 attacks, Board *board) {
    int toSq;
    while (attacks) {
        toSq = poplsb(&attacks);
        if (board->squares[toSq] == EMPTY)
            moves->list[moves->count] = ConstructMove(fromSq, toSq, QUIET_FLAG);
        else
            moves->list[moves->count] = ConstructMove(fromSq, toSq, CAPTURE_FLAG);
        moves->count++;
    }
}

static inline void addCaptures(MoveList *moves, int fromSq, U64 captures, Board *board) {
    int toSq;
    while (captures) {
        toSq = poplsb(&captures);
        moves->list[moves->count] = ConstructMove(fromSq, toSq, CAPTURE_FLAG);
        moves->count++;
    }
}

static inline void addPawnCaptures(int epSquare, MoveList *moves, int fromSq, U64 attacks) {
    int toSq;
    while (attacks) {
        toSq = poplsb(&attacks);
        if (toSq == epSquare) {
            moves->list[moves->count] = ConstructMove(fromSq, toSq, EP_FLAG);
            moves->count++;
        } else {
            moves->list[moves->count] = ConstructMove(fromSq, toSq, CAPTURE_FLAG);
            moves->count++;
        }
    }
}

static inline void addCastleMove(MoveList *moves, int fromSq, int toSq) {
    moves->list[moves->count] = ConstructMove(fromSq, toSq, CASTLE_FLAG);
    moves->count++;
}

// Given a bitboard of pawns which were pushed and how far they were pushed,
// adds all those moves
static inline void addPawnPushes(MoveList *moves, U64 pushes, int side, int pushAmount) {
    int toSq;
    int pawnDeltas[2] = {-8, 8};
    while (pushes) {
        toSq = poplsb(&pushes);
        moves->list[moves->count] = ConstructMove(toSq + (pawnDeltas[side] * pushAmount), toSq, QUIET_FLAG);
        moves->count++;
    }
}

static inline void addPromotionPushes(MoveList *moves, U64 pushes, int side) {
    int toSq;
    int pawnDeltas[2] = {-8, 8};
    while (pushes) {
        toSq = poplsb(&pushes);
        moves->list[moves->count] = ConstructMove(toSq + pawnDeltas[side], toSq, KNIGHT_PROMO_FLAG);
        moves->count++;
        moves->list[moves->count] = ConstructMove(toSq + pawnDeltas[side], toSq, BISHOP_PROMO_FLAG);
        moves->count++;
        moves->list[moves->count] = ConstructMove(toSq + pawnDeltas[side], toSq, ROOK_PROMO_FLAG);
        moves->count++;
        moves->list[moves->count] = ConstructMove(toSq + pawnDeltas[side], toSq, QUEEN_PROMO_FLAG);
        moves->count++;
    }
}

static inline void addPromotionCaptures(MoveList *moves, U64 captures, int from) {
    int to;
    while (captures) {
        // Add all four promotion pieces bitwise or with capture flag
        to = poplsb(&captures);
        moves->list[moves->count] = ConstructMove(from, to, KNIGHT_PROMO_FLAG | CAPTURE_FLAG);
        moves->count++;
        moves->list[moves->count] = ConstructMove(from, to, BISHOP_PROMO_FLAG | CAPTURE_FLAG);
        moves->count++;
        moves->list[moves->count] = ConstructMove(from, to, ROOK_PROMO_FLAG | CAPTURE_FLAG);
        moves->count++;
        moves->list[moves->count] = ConstructMove(from, to, QUEEN_PROMO_FLAG | CAPTURE_FLAG);
        moves->count++;
    }
}

static inline void generatePawnMoves(MoveList *moves, Board *board) {
    U64 doublePushRanks[2] = {RANK_2, RANK_7};
    U64 promotionRanks[2] = {RANK_8, RANK_1};

    U64 pawns = board->pieces[PAWN] & board->colors[board->side];
    U64 emptySquares = ~(board->colors[BOTH]);
    U64 pushes, doublePushes;
    U64 attacks;

    if (board->side == WHITE) {
        // Generate pawn pushes via bitshifting
        pushes = (pawns << 8) & emptySquares;

        // Double pushes
        doublePushes = ((pawns & doublePushRanks[board->side]) << 16) & emptySquares;

        // Remove double pushes blocked
        doublePushes = doublePushes & emptySquares << 8;
    } else {
        // Generate pawn pushes via bitshifting
        pushes = (pawns >> 8) & emptySquares;

        // Double pushes
        doublePushes = ((pawns & doublePushRanks[board->side]) >> 16) & emptySquares;

        // Remove double pushes blocked
        doublePushes = doublePushes & emptySquares >> 8;
    }
    addPawnPushes(moves, doublePushes, board->side, 2);

    // Sort pushes between promotions and quiet pushes before adding
    U64 quietPushes = pushes & ~promotionRanks[board->side];
    addPawnPushes(moves, quietPushes, board->side, 1);
    U64 promotionPushes = pushes & promotionRanks[board->side];
    addPromotionPushes(moves, promotionPushes, board->side);

    // Generate pawn captures, with promotions and en passant

    // Attackable squares = enemy pieces or en passant square
    U64 attackable;
    attackable = board->colors[!board->side];
    if (board->epSquare != NO_SQ) {
        setBit(&attackable, board->epSquare);
    }

    while (pawns) {
        int from = poplsb(&pawns);
        attacks = pawnAttacks(board->side, from) & attackable;
        // Promotions
        if (attacks & promotionRanks[board->side]) {
            addPromotionCaptures(moves, attacks, from);
        }
        // Normal captures
        else {
            addPawnCaptures(board->epSquare, moves, from, attacks);
        }
    }
}

static inline void generateKingMoves(MoveList *moves, Board *board) {
    // Get the king's square
    int kingSq = getlsb(board->pieces[KING] & board->colors[board->side]);
    U64 attacks = kingAttacks(kingSq) & ~board->colors[board->side];

    // Add king attacks from that square
    addNormalMoves(moves, kingSq, attacks, board);

    // If in check, break out and don't check castling
    if (isSquareAttacked(board, board->side, kingSq))
        return;

    // Add castling moves
    if (board->side == WHITE) {
        // White's Kingside Castling
        if (board->castlePerm & CASTLE_WK) {
            if ((board->colors[BOTH] & CASTLE_MASK_WK) == 0ull) {
                if (!isSquareAttacked(board, WHITE, F1) && !isSquareAttacked(board, WHITE, G1))
                    addCastleMove(moves, E1, G1);
            }
        }
        // White's Queenside Castling
        if (board->castlePerm & CASTLE_WQ) {
            if ((board->colors[BOTH] & CASTLE_MASK_WQ) == 0ull) {
                if (!isSquareAttacked(board, WHITE, D1) && !isSquareAttacked(board, WHITE, C1))
                    addCastleMove(moves, E1, C1);
            }
        }
    } else {
        // Black's Kingside Castling
        if (board->castlePerm & CASTLE_BK) {
            if ((board->colors[BOTH] & CASTLE_MASK_BK) == 0ull) {
                if (!isSquareAttacked(board, BLACK, F8) && !isSquareAttacked(board, BLACK, G8))
                    addCastleMove(moves, E8, G8);
            }
        }
        // Black's Queenside Castling
        if (board->castlePerm & CASTLE_BQ) {
            if ((board->colors[BOTH] & CASTLE_MASK_BQ) == 0ull) {
                if (!isSquareAttacked(board, BLACK, D8) && !isSquareAttacked(board, BLACK, C8))
                    addCastleMove(moves, E8, C8);
            }
        }
    }
}

static inline void generateKnightMoves(MoveList *moves, Board *board) {
    // Get knights on our side
    U64 knights = board->pieces[KNIGHT] & board->colors[board->side];
    U64 attacks;

    // Loop through all of them, looking up their attacks
    while (knights) {
        int from = poplsb(&knights);

        attacks = knightAttacks(from) & ~board->colors[board->side];
        addNormalMoves(moves, from, attacks, board);
    }
}

static inline void generateSlidingMoves(MoveList *moves, Board *board) {
    // Bishops and queens
    U64 bishops = (
        board->pieces[BISHOP] | board->pieces[QUEEN]
    ) & board->colors[board->side];

    while (bishops) {
        int from = poplsb(&bishops);

        U64 attacks = Bmagic(from, board->colors[BOTH]) & ~board->colors[board->side];
        addNormalMoves(moves, from, attacks, board);
    }

    // Rooks and queens
    U64 rooks = (
        board->pieces[ROOK] | board->pieces[QUEEN]
    ) & board->colors[board->side];
    while (rooks) {
        int from = poplsb(&rooks);

        U64 attacks = Rmagic(from, board->colors[BOTH]) & ~board->colors[board->side];
        addNormalMoves(moves, from, attacks, board);
    }
}

void generatePseudoLegalMoves(MoveList *moves, Board *board) {
    moves->count = 0;

    // If double check we exit early
    U64 kingCheckers = attackersToKingSquare(board);
    int checkerCount = popCount(kingCheckers);
    if (checkerCount >= 2) {
        // Just generate king moves
        generateKingMoves(moves, board);
        return;
    }

    // Pawns
    generatePawnMoves(moves, board);

    // Sliders
    generateSlidingMoves(moves, board);

    // Knights and King
    generateKnightMoves(moves, board);
    generateKingMoves(moves, board);
}

void generateLegalMoves(MoveList *moves, Board *board) {
    // NOTE: Should not be used, it's highly inefficient
    // Generally we just check legality as we make the moves

    MoveList tempMoves;
    generatePseudoLegalMoves(&tempMoves, board);

    moves->count = 0;

    for (int i = 0; i < tempMoves.count; i++) {
        if (makeMove(board, tempMoves.list[i]) == 0) {
            undoMove(board, tempMoves.list[i]);
            continue;
        }
        moves->list[moves->count] = tempMoves.list[i];
        moves->count++;
        undoMove(board, tempMoves.list[i]);
    }
}

void printMoveList(MoveList moves) {
    // Debug function:
    // Prints move and move type of all moves in a movelist

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.list[i];
        printf("%d: ", i);
        printMove(move, 0);

        if (IsCapture(move)) {
            printf(" Capture");
        }
        if (IsCastling(move)) {
            printf(" Castling");
        }

        if (IsEnpass(move)) {
            printf(" En passant");
        }
        if (IsPromotion(move)) {
            printf(" Promotion");
        }
        printf("\n");
    }
    printf("Count: %d\n", moves.count);
}
