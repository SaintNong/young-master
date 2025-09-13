#pragma once

#include <stdint.h>


typedef uint16_t Move;

/*
 *   16-bit move representation breakdown
 *
 *   0000 0000 0000 0000 |
 *   0000 0000 0011 1111 | Move origin square      (6 bits => 64 possible)
 *   0000 1111 1100 0000 | Move destination square (6 bits => 64 possible)
 *   1111 0000 0000 0000 | Flags                   (4 bits => 16 possible)
 */

// Represents a move that doesn't exist, e.g. unfilled parts of a hash table,
// or a null move.
#define NO_MOVE 0

// Move flags
#define QUIET_FLAG          0x0
#define CASTLE_FLAG         0x1
#define CAPTURE_FLAG        0x4
#define EP_FLAG             0x6
#define PROMO_FLAG          0x8
#define KNIGHT_PROMO_FLAG   0x8
#define BISHOP_PROMO_FLAG   0x9
#define ROOK_PROMO_FLAG     0xA
#define QUEEN_PROMO_FLAG    0xB

#define ConstructMove(from, to, flags) (from) | ((to) << 6) | ((flags) << 12)

#define MoveFrom(move)          ((move) & 0x3F)
#define MoveTo(move)            (((move) & 0xFC0) >> 6)
#define MoveFlags(move)         (((move) & 0xF000) >> 12)

#define IsCapture(move)         (!!(MoveFlags(move) & CAPTURE_FLAG))
#define IsQuiet(move)           (MoveFlags(move) == QUIET_FLAG)
#define IsEnpass(move)          ((MoveFlags(move) == EP_FLAG))
#define IsCastling(move)        ((MoveFlags(move) == CASTLE_FLAG))
#define IsPromotion(move)       (!!(MoveFlags(move) & PROMO_FLAG))
#define MovePromotedPiece(move) ((MoveFlags(move) & 0x3) + 1)

// Move functions
void printMove(Move move, bool includeNewLine);
char* moveToString(Move move);
