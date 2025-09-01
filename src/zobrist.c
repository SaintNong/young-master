#include "zobrist.h"

#include "bitboards.h"
#include "board.h"
#include "utils.h"

U64 PieceKeys[12][64];
U64 EpKeys[64];
U64 CastleKeys[16];
U64 SideKey;

// Generates a zobrist hash from a board
// Very slow, so ideally only called after initialising the board
U64 generateHash(Board *board) {
    U64 hash = 0ULL;

    // Apply piece keys
    for (int sq = 0; sq < 64; sq++) {
        if (board->squares[sq] != EMPTY) {
            if (testBit(board->colors[WHITE], sq))
                hash ^= PieceKeys[toPiece(board->squares[sq], WHITE)][sq];
            else
                hash ^= PieceKeys[toPiece(board->squares[sq], BLACK)][sq];
        }
    }

    // Apply en passant key
    if (board->epSquare != NO_SQ)
        hash ^= EpKeys[board->epSquare];

    // Apply castle key
    hash ^= CastleKeys[board->castlePerm];

    // Apply side key
    if (board->side == BLACK)
        hash ^= SideKey;

    return hash;
}

void initZobristKeys() {
    // Piece keys
    for (int piece = PAWN; piece < NB_PIECES; piece++) {
        for (int sq = 0; sq < 64; sq++) {
            // Make piece key for every color for every square
            PieceKeys[toPiece(piece, WHITE)][sq] = randomU64();
            PieceKeys[toPiece(piece, BLACK)][sq] = randomU64();
        }
    }

    // En passant keys
    for (int sq = 0; sq < 64; sq++)
        EpKeys[sq] = randomU64();

    // Castle keys
    for (int castlePerm = 0; castlePerm < CASTLE_MAX; castlePerm++)
        CastleKeys[castlePerm] = randomU64();

    // Side key
    SideKey = randomU64();
}