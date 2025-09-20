#pragma once

#include "board.h"

int perftDivide(Board *board, int depth);
void perftBench(Board *board, int depth);
void perftSuite();

typedef struct {
    char *fen;
    int depth;
    U64 nodes;
} PerftEntry;


/**
 * Perft test suite, and benchmark suite
 * Source: 
 * - Random positions from FIDE Grand Swiss 2025
 * - Positions from this page: https://www.chessprogramming.org/Perft_Results
 * - Some from: https://github.com/ChrisWhittington/Chess-EPDs/blob/master/perft.epd
 */
#define PERFT_POSITION_COUNT 14
static const PerftEntry PERFT_TESTS[PERFT_POSITION_COUNT] = {
    {
        // Start position
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        5,
        4865609
    },
    {
        // Bluebaum vs Firouzja Round 11
        "1n1q1rk1/1bpp2bp/1p2p1p1/r4p2/P1PPn3/1QN1PN2/4BPPP/R1B2RK1 w - - 2 12",
        4,
        3119583
    },
    {
        // Giri vs Niemann Round 11
        "r1bqk2r/pppp1ppp/2n2n2/4p3/1bP5/2N1PN2/PP1P1PPP/R1BQKB1R w KQkq - 1 5",
        5,
        35792930
    },
    {
        // Giri vs Niemann Round 11
        "8/p3k1pp/B2r1p2/2B5/4P1PP/Pb3P2/5K2/8 b - - 0 34",
        5,
        6168063
    },
    {
        // Sarin vs Maghsoodloo Round 7
        "4r1k1/5p2/5p2/Q2p4/P6N/1r3PqP/3RB1P1/5K2 w - - 0 38",
        5,
        38451395
    },
    {
        // Niemann vs Praggnanandhaa Round 10
        "8/2r3kp/p2p1pp1/3B4/1R2Q3/P5PP/1P2bP1K/5q2 w - - 5 35",
        5,
        62928370
    },
    {
        // Kiwipete position
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
        4,
        4085603
    },
    {
        // En passant stopped because of check
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        5,
        674624
    },
    {
        // Crazy position with promotions and stuff
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        5,
        15833292
    },
    {
        // Captured rook cannot castle
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        5,
        89941194
    },
    // === Positions from perft.epd ===
    {
        "r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1",
        5,
        7848606
    },
    {
        "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1",
        5,
        31912360
    },
    {
        "8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1",
        8,
        7594587
    },
    {
        "1nnk1n2/2qrpp2/8/4RP2/4KN2/4Q3/8/8 w - - 0 1",
        5,
        20550721
    }
};
