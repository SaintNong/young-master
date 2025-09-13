#include <assert.h>
#include <stdio.h>

#include "makemove.h"
#include "board.h"
#include "move.h"
#include "zobrist.h"

/* -------------------------------------------------------------------------- */
/*                              Castling Helpers                              */
/* -------------------------------------------------------------------------- */

static inline void RemoveWhiteCastling(Board *board) {
    board->hash ^= CastleKeys[board->castlePerm];
    board->castlePerm &= ~(CASTLE_WK | CASTLE_WQ);
    board->hash ^= CastleKeys[board->castlePerm];
}

static inline void RemoveBlackCastling(Board *board) {
    board->hash ^= CastleKeys[board->castlePerm];
    board->castlePerm &= ~(CASTLE_BK | CASTLE_BQ);
    board->hash ^= CastleKeys[board->castlePerm];
}

static inline void RemoveWhiteKingCastling(Board *board) {
    board->hash ^= CastleKeys[board->castlePerm];
    board->castlePerm &= ~CASTLE_WK;
    board->hash ^= CastleKeys[board->castlePerm];
}

static inline void RemoveWhiteQueenCastling(Board *board) {
    board->hash ^= CastleKeys[board->castlePerm];
    board->castlePerm &= ~CASTLE_WQ;
    board->hash ^= CastleKeys[board->castlePerm];
}

static inline void RemoveBlackKingCastling(Board *board) {
    board->hash ^= CastleKeys[board->castlePerm];
    board->castlePerm &= ~CASTLE_BK;
    board->hash ^= CastleKeys[board->castlePerm];
}

static inline void RemoveBlackQueenCastling(Board *board) {
    board->hash ^= CastleKeys[board->castlePerm];
    board->castlePerm &= ~CASTLE_BQ;
    board->hash ^= CastleKeys[board->castlePerm];
}

/* -------------------------------------------------------------------------- */
/*                           Board mutation helpers                           */
/* -------------------------------------------------------------------------- */
/**
 * These functions are carbon copies of the functions in board.c, but without
 * hash updates since they are unnecessary in undoMove. They make undoMove much
 * more performant, although at the cost of code duplication.
 * 
 * TODO: do something about these ugly functions :/
 */

// setPiece but without updating the hash
// for use in undo where hash is reverted from a saved value
static inline void setPieceNoHash(Board *board, int color, int piece, int sq) {
    assert(board->squares[sq] == EMPTY);      // Square is empty
    assert(piece >= PAWN && piece <= KING);   // Valid piece
    assert(color == WHITE || color == BLACK); // Valid color
    assert(sq >= A1 && sq <= H8);             // Valid square

    // Set piece on pieces bitboard, colors[color] and colors[BOTH]
    setBit(&board->pieces[piece], sq);
    setBit(&board->colors[color], sq);
    setBit(&board->colors[BOTH], sq);

    // Set piece on mailbox board
    board->squares[sq] = piece;
}

// ClearPiece but without updating the hash
// for use in undo where hash is reverted from a saved value
static inline void clearPieceNoHash(Board *board, int color, int sq) {
    int piece = board->squares[sq];

    assert(piece >= PAWN && piece <= KING);   // Valid piece
    assert(color == WHITE || color == BLACK); // Valid color
    assert(sq >= A1 && sq <= H8);             // Valid square

    // Clear piece on mailbox board
    board->squares[sq] = EMPTY;

    // Clear piece from bitboards
    clearBit(&board->pieces[piece], sq);
    clearBit(&board->colors[color], sq);
    clearBit(&board->colors[BOTH], sq);
}

// movePiece but without updating the hash
// for use in undo where hash is reverted from a saved value
void movePieceNoHash(Board *board, int from, int to, int color) {
    // Clear the from square
    int piece = board->squares[from];

    assert(board->squares[to] == EMPTY);      // to sq empty
    assert(piece >= PAWN && piece <= KING);   // Valid piece
    assert(color == WHITE || color == BLACK); // Valid color
    assert(from >= A1 && from <= H8);         // Valid from square
    assert(to >= A1 && to <= H8);             // Valid to square

    board->squares[from] = EMPTY;

    // Clear piece from bitboards
    clearBit(&board->pieces[piece], from);
    clearBit(&board->colors[color], from);
    clearBit(&board->colors[BOTH], from);

    // Set piece on pieces bitboard, colors[color] and colors[BOTH]
    setBit(&board->pieces[piece], to);
    setBit(&board->colors[color], to);
    setBit(&board->colors[BOTH], to);

    // Set on mailbox board
    board->squares[to] = piece;
}

/* -------------------------------------------------------------------------- */
/*                                 Null Moves                                 */
/* -------------------------------------------------------------------------- */

// Makes a Null move on the board (basically switches the side and thats it).
void makeNullMove(Board *board) {
    // Save hard to compute values
    Undo *undo = &board->history[board->hisPly++];
    undo->castlePerm = board->castlePerm;
    undo->epSquare = board->epSquare;
    undo->fiftyMove = board->fiftyMove;
    undo->hash = board->hash;
    undo->move = NO_MOVE;

    // Update side to move and ply
    board->side = !board->side;
    board->hash ^= SideKey;

    // Clear en passant square if there was one
    if (board->epSquare != NO_SQ) {
        board->hash ^= EpKeys[board->epSquare];
        board->epSquare = NO_SQ;
    }

    // Null moves are irreversible right?
    board->fiftyMove = 0;

    assert(board->hash == generateHash(board));
}

// Undoes a null move (Be careful to only call after you made a null move!)
void undoNullMove(Board *board) {
    // Make sure the last move was actually a null move
    assert(board->history[board->hisPly - 1].move == NO_MOVE);

    // Swap the turn back
    board->hisPly--;
    board->side = !board->side;

    // Revert information
    Undo *undo = &board->history[board->hisPly];
    board->castlePerm = undo->castlePerm;
    board->epSquare = undo->epSquare;
    board->fiftyMove = undo->fiftyMove;
    board->hash = undo->hash;

    assert(board->hash == generateHash(board));
}

/* -------------------------------------------------------------------------- */
/*                                  Undo Move                                 */
/* -------------------------------------------------------------------------- */

// Undoes the previous move on the board
void undoMove(Board *board, Move move) {
    // Decode move
    int to = MoveTo(move);
    int from = MoveFrom(move);

    // Easy reversions
    board->side = !board->side;
    board->hisPly--;

    // Revert values which are hard to compute
    Undo *undo = &board->history[board->hisPly];
    board->castlePerm = undo->castlePerm;
    board->epSquare = undo->epSquare;
    board->fiftyMove = undo->fiftyMove;
    board->hash = undo->hash;

    int capturedPiece = undo->capturedPiece;
    int movedPiece = undo->movedPiece;

    // Clear the destination square of the move
    clearPieceNoHash(board, board->side, to);

    // Put the piece back where it came from
    setPieceNoHash(board, board->side, movedPiece, from);

    // Deal with castling and captures
    if (IsCapture(move)) {
        assert(capturedPiece >= PAWN && capturedPiece <= KING);

        // Either en passant or normal capture
        if (IsEnpass(move)) {
            // Put the en passanted (is that even a verb?) pawn back
            int capturedSquare = (board->side == WHITE) ? (to - 8) : (to + 8);
            setPieceNoHash(board, !board->side, PAWN, capturedSquare);
        } else {
            // Put the captured piece back
            setPieceNoHash(board, !board->side, capturedPiece, to);
        }
    } else if (IsCastling(move)) {
        // Put the rook back
        switch (to) {
        case C1: // White queenside
            movePieceNoHash(board, D1, A1, WHITE);
            break;
        case G1: // White kingside
            movePieceNoHash(board, F1, H1, WHITE);
            break;
        case C8: // Black queenside
            movePieceNoHash(board, D8, A8, BLACK);
            break;
        case G8: // Black kingside
            movePieceNoHash(board, F8, H8, BLACK);
            break;
        }
    }

    assert(board->hash == generateHash(board));
}

/* -------------------------------------------------------------------------- */
/*                                  Make Move                                 */
/* -------------------------------------------------------------------------- */

// Checks if the opponent is currently in check on our turn.
static inline int moveWasIllegal(Board *board) {
    return isSquareAttacked(board, !board->side,
        getlsb(
        board->pieces[KING] & board->colors[!board->side]
    ));
}

/**
 * Makes the pseudolegal move on the board, returns true if it was legal,
 * false if it was not.
 */
int makeMove(Board *board, Move move) {
    // Extract information from the move
    int from = MoveFrom(move);
    int to = MoveTo(move);
    int movedPiece = board->squares[from];

    assert(movedPiece >= PAWN && movedPiece <= KING);

    // Save information which is hard to recompute when undoing
    Undo *undo = &board->history[board->hisPly++];
    undo->castlePerm = board->castlePerm;
    undo->epSquare = board->epSquare;
    undo->fiftyMove = board->fiftyMove;
    undo->movedPiece = movedPiece;
    undo->hash = board->hash;
    undo->capturedPiece = NO_PIECE;
    undo->move = move;

    // Clear en passant square information
    if (board->epSquare != NO_SQ) {
        board->hash ^= EpKeys[board->epSquare];
        board->epSquare = NO_SQ;
    }

    // Remove castle rights if capturing a rook
    if (IsCapture(move) && board->squares[to] == ROOK) {
        if (board->side == WHITE) {
            if (to == A8) {
                RemoveBlackQueenCastling(board);
            } else if (to == H8) {
                RemoveBlackKingCastling(board);
            }
        } else {
            if (to == A1) {
                RemoveWhiteQueenCastling(board);
            } else if (to == H1) {
                RemoveWhiteKingCastling(board);
            }
        }
    }

    // Handle moves by type
    if (IsCastling(move)) {
        // Perform castling move: Move both king and rook
        switch (to) {
        case C1: // White queenside
            movePiece(board, E1, C1, WHITE);
            movePiece(board, A1, D1, WHITE);
            RemoveWhiteCastling(board);
            break;
        case G1: // White kingside
            movePiece(board, E1, G1, WHITE);
            movePiece(board, H1, F1, WHITE);
            RemoveWhiteCastling(board);
            break;
        case C8: // Black queenside
            movePiece(board, E8, C8, BLACK);
            movePiece(board, A8, D8, BLACK);
            RemoveBlackCastling(board);
            break;
        case G8: // Black kingside
            movePiece(board, E8, G8, BLACK);
            movePiece(board, H8, F8, BLACK);
            RemoveBlackCastling(board);
            break;
        }
    } else if (IsEnpass(move)) {
        // Move the pawn to the en passant square
        movePiece(board, from, to, board->side);

        // Clear the captured pawn
        int capturedSquare = (board->side == WHITE) ? (to - 8) : (to + 8);
        clearPiece(board, !board->side, capturedSquare);

        undo->capturedPiece = PAWN;
    } else if (IsPromotion(move)) {
        // Clear the pawn and add the new piece
        clearPiece(board, board->side, from);

        // If capture, remove the captured piece
        if (IsCapture(move)) {
            undo->capturedPiece = board->squares[to];
            clearPiece(board, !board->side, to);
        }

        int promotedPiece = MovePromotedPiece(move);
        setPiece(board, board->side, promotedPiece, to);
    } else {
        // Regular move or capture
        if (IsCapture(move)) {
            undo->capturedPiece = board->squares[to];
            clearPiece(board, !board->side, to); // Remove the captured piece
        }
        // Move the piece
        setPiece(board, board->side, board->squares[from], to);
        clearPiece(board, board->side, from);

        // Last move was a pawn double push
        if ((movedPiece == PAWN) && ((from ^ to) == 16)) {
            // Set epSquare to average from and to
            board->epSquare = (from + to) / 2;
            board->hash ^= EpKeys[board->epSquare];
        } else if (movedPiece == KING) {
            // Remove castle rights
            if (board->side == WHITE)
                RemoveWhiteCastling(board);
            else
                RemoveBlackCastling(board);
        } else if (movedPiece == ROOK) {
            if (board->side == WHITE) {
                if (from == A1)
                    RemoveWhiteQueenCastling(board);
                else if (from == H1)
                    RemoveWhiteKingCastling(board);
            } else {
                if (from == A8)
                    RemoveBlackQueenCastling(board);
                else if (from == H8)
                    RemoveBlackKingCastling(board);
            }
        }
    }

    // Handle 50 move rule counter
    if (movedPiece == PAWN || IsCapture(move)) {
        board->fiftyMove = 0; // Reset on pawn move or capture
    } else {
        board->fiftyMove++; // Increment otherwise
    }

    // Update side to move
    board->side = !board->side;
    board->hash ^= SideKey;

    assert(board->hash == generateHash(board));

    // If we're in check, that move was illegal
    if (moveWasIllegal(board))
        return 0;
    else
        return 1;
}
