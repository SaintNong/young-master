#pragma once

#include <stdbool.h>

#include "bitboards.h"
#include "move.h"

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"


/* -------------------------------------------------------------------------- */
/*                       Board Representation Constants                       */
/* -------------------------------------------------------------------------- */

// 4-bit Castle Flags (KQkq)
enum {
    CASTLE_WK = 1,
    CASTLE_WQ = 2,
    CASTLE_BK = 4,
    CASTLE_BQ = 8,
    CASTLE_MAX = 16
};

// Little endian square representation
#define MIRROR_SQ(sq) ((sq) ^ 56)
enum {
    A1, B1, C1, D1, E1, F1, G1, H1, 
    A2, B2, C2, D2, E2, F2, G2, H2, 
    A3, B3, C3, D3, E3, F3, G3, H3, 
    A4, B4, C4, D4, E4, F4, G4, H4, 
    A5, B5, C5, D5, E5, F5, G5, H5, 
    A6, B6, C6, D6, E6, F6, G6, H6, 
    A7, B7, C7, D7, E7, F7, G7, H7, 
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQ
};

enum {
    RANK_1 = 0x00000000000000FFULL,
    RANK_2 = 0x000000000000FF00ULL,
    RANK_3 = 0x0000000000FF0000ULL,
    RANK_4 = 0x00000000FF000000ULL,
    RANK_5 = 0x000000FF00000000ULL,
    RANK_6 = 0x0000FF0000000000ULL,
    RANK_7 = 0x00FF000000000000ULL,
    RANK_8 = 0xFF00000000000000ULL
};

// converts (piece, color) to colored piece
#define toPiece(piece, color) ((piece) + ((color) * 6))
enum {
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    EMPTY,
    NO_PIECE,
    NB_PIECES = 6
};

#define MAX_MOVES 2048

enum { WHITE, BLACK, BOTH };

// Hard to recompute information for undoing moves
typedef struct {
    int castlePerm;
    int epSquare;
    int fiftyMove;
    int movedPiece;
    int capturedPiece;
    Move move;

    U64 hash;
} Undo;

// Chess Board Representation
typedef struct {
    U64 colors[3];           // Occupancies for colors WHITE, BLACK and BOTH
    U64 pieces[6];           // Colourblind bitboards for each piece
    int squares[64];         // Array of what piece is on what square

    int side;                // WHITE | BLACK
    int epSquare;            // En-passant square - NO_SQ if none exists
    int castlePerm;          // Castle permissions for both sides in 4 bits (KQkq)
    int fiftyMove;           // 50 move rule counter
    int hisPly;              // Half moves since start of game, index of repetition table

    U64 hash;                // Zobrist hash

    Undo history[MAX_MOVES]; // List of possible undos to past positions
} Board;


// Square, rank, and file helpers
int squareFrom(int file, int rank);
int rankOf(int sq);
int fileOf(int sq);
bool fileRankInBoard(int file, int rank);

void squareToString(int square, char *coords);
int stringToSquare(char *string);

// Board functions
// Mutations
void setPiece(Board *board, int color, int piece, int sq);
void clearPiece(Board *board, int color, int sq);
void movePiece(Board *board, int from, int to, int color);

// Attacks
int isSquareAttacked(Board *board, int color, int square);
U64 allAttackersToSquare(Board *board, U64 occupied, int sq);
U64 attackersToKingSquare(Board *board);
void clearBoard(Board *board);

bool isDraw(Board *board);
// Board IO
void parseFen(Board *board, char *fen);
void printBoard(Board *board);
