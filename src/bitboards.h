#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t U64;

// LSB and MS
int poplsb(U64 *bitboard);
int popmsb(U64 *bitboard);
int getlsb(U64 bitboard);
int getmsb(U64 bitboard);

// Bitboard operations
int popCount(U64 bitboard);
void setBit(U64 *bitboard, int sq);
void clearBit(U64 *bitboard, int sq);
bool testBit(U64 bitboard, int sq);
bool multipleBits(U64 bitboard);

// Helper functions for IO
void printBitboard(U64 bitboard);

// initialises attack masks for Kings, Knights and Pawns
void initAttackMasks();

// Wrappers functions to 3 lookup tables
U64 knightAttacks(int sq);
U64 kingAttacks(int sq);
U64 pawnAttacks(int color, int sq);
