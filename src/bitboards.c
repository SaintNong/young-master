#include "bitboards.h"

#include <stdbool.h>
#include <stdio.h>

#include "board.h"

// Private tables for use in this function only
U64 knightMasks[64];
U64 kingMasks[64];
U64 pawnMasks[2][64];

// bitboard operations
int getlsb(U64 bitboard) {
    return __builtin_ctzll(bitboard);
}

int getmsb(U64 bitboard) {
    return __builtin_ctzll(bitboard) ^ 63;
}

int poplsb(U64 *bitboard) {
    int lsb = getlsb(*bitboard);
    *bitboard &= *bitboard - 1;
    return lsb;
}

int popmsb(U64 *bitboard) {
    int msb = getmsb(*bitboard);
    *bitboard ^= 1ULL << msb;
    return msb;
}

void setBit(U64 *bitboard, int sq) {
    *bitboard |= 1ULL << sq;
}

void clearBit(U64 *bitboard, int sq) {
    *bitboard &= ~(1ULL << sq);
}

int popCount(U64 bitboard) {
    return __builtin_popcountll(bitboard);
}

bool testBit(U64 bitboard, int sq) {
    return bitboard & (1ULL << sq);
}

// Displays bitboard with chess coordinates
// Set bits are marked 'X'
void printBitboard(U64 bitboard) {
    for (int rank = 7; rank >= 0; --rank) {
        printf("%d ", rank + 1);

        for (int file = 0; file < 8; ++file) {
            int sq = squareFrom(file, rank);
            printf("%c ", testBit(bitboard, sq) ? 'X' : '.');
        }

        printf("\n");
    }
    printf("  a b c d e f g h\n");
    printf("  Popcount: %d\n", popCount(bitboard));
}

// Mask generation for non-sliding pieces
U64 createKnightMask(int sq) {
    const int DIRECTIONS[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
                                  { 1, -2}, { 1, 2}, { 2, -1}, { 2, 1}};
    U64 mask = 0ULL;

    // break square down to file and rank
    int file = fileOf(sq);
    int rank = rankOf(sq);
    int endFile;
    int endRank;

    // permute through all directions
    for (int dir = 0; dir < 8; ++dir) {
        endRank = rank + DIRECTIONS[dir][0];
        endFile = file + DIRECTIONS[dir][1];

        // if the end square is on the board, set the bit
        if (fileRankInBoard(endFile, endRank))
            setBit(&mask, squareFrom(endFile, endRank));
    }
    return mask;
}

U64 createKingMask(int sq) {
    const int DIRECTIONS[8][2] = {{-1, -1}, {-1,  0}, {-1, 1}, {0, -1},
                                  { 0,  1}, { 1, -1}, {1,  0}, {1,  1}};
    U64 mask = 0ULL;

    // break square down to file and rank
    int file = fileOf(sq);
    int rank = rankOf(sq);
    int endFile;
    int endRank;

    // permute through all directions
    for (int dir = 0; dir < 8; ++dir) {
        endRank = rank + DIRECTIONS[dir][0];
        endFile = file + DIRECTIONS[dir][1];

        // if the end square is on the board, set the bit
        if (fileRankInBoard(endFile, endRank))
            setBit(&mask, squareFrom(endFile, endRank));

    }
    return mask;
}

U64 createPawnMask(int color, int sq) {
    const int DIRECTIONS[2][2] = {{1, 1}, {1, -1}};
    U64 mask = 0ULL;

    // break square down to file and rank
    int file = fileOf(sq);
    int rank = rankOf(sq);
    int endFile;
    int endRank;

    // permute through all directions
    for (int dir = 0; dir < 2; ++dir) {
        // if the color is black, the pawns go downwards
        if (color == WHITE)
            endRank = rank + DIRECTIONS[dir][0];
        else
            endRank = rank - DIRECTIONS[dir][0];
        endFile = file + DIRECTIONS[dir][1];

        // if the end square is on the board, set the bit
        if (fileRankInBoard(endFile, endRank))
            setBit(&mask, squareFrom(endFile, endRank));
    }

    return mask;
}

// initialises attack masks for Kings, Knights and Pawns
void initAttackMasks() {
    for (int sq = 0; sq < 64; ++sq) {
        knightMasks[sq] = createKnightMask(sq);
        kingMasks[sq] = createKingMask(sq);

        pawnMasks[WHITE][sq] = createPawnMask(WHITE, sq);
        pawnMasks[BLACK][sq] = createPawnMask(BLACK, sq);
    }
}

// wrapper functions to access masks
U64 knightAttacks(int sq) { return knightMasks[sq]; }

U64 kingAttacks(int sq) { return kingMasks[sq]; }

U64 pawnAttacks(int color, int sq) { return pawnMasks[color][sq]; }
