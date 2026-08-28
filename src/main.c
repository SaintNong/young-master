#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "uci.h"
#include "utils.h"
#include "bitboards.h"
#include "magicmoves.h"
#include "movepicker.h"
#include "zobrist.h"
#include "hashtable.h"
#include "search.h"
#include "perft.h"
#include "eval.h"
#include "bench.h"
#include "see_test.h"

#define NAME_VERSION_STRING WHT NAME " [" CYN VERSION WHT "]" CRESET
void welcome() {
    // Print versioning info
    // printf("Length %lu\n", strlen(NAME_VERSION_STRING));
    puts("=======================================");
    printf("|  %s        |\n", NAME_VERSION_STRING);
    printf("|  "WHT "Compiled on: %s" CRESET"  |\n", __DATE__ " " __TIME__);
    puts("=======================================");

    // Print an arrogant message
    puts(" >  The Young Master is suddenly interrupted during his seclusion.");
    puts(" >  A mere patzer like you dares challenge this young master's dao of chess?");
    puts(" >  You are courting death! Prepare to have your foundation shattered and meridians severed.\n");
}

void initialise() {
    // Movegen
    initmagicmoves();
    initAttackMasks();

    // Move ordering
    initMvvLva();
    
    // Hash table
    initZobristKeys();
    initHashTable(HASH_SIZE_DEFAULT);

    // Search
    initSearchTables();

    // Evaluation
    initEvaluation();
}

/**
 * Evaluates every position in a file; used to check for total evaluation parity
 * between this engine and the texel tuner's C++ implementation.
 */
static int evalFile(const char *path, long limit) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("Unable to open evaluation file");
        return EXIT_FAILURE;
    }

    Board board;
    char fen[4096];
    long index = 0;
    while ((limit <= 0 || index < limit) && fgets(fen, sizeof(fen), file) != NULL) {
        parseFen(&board, fen);
        printf("EVAL %ld %d\n", index, evaluate(&board));
        index++;
    }

    fclose(file);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    welcome();
    initialise();

    if (argc != 1) {
        if (strcmp(argv[1], "bench") == 0) {
            bench();
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[1], "perft-test") == 0) {
            return perftSuite() ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        if (strcmp(argv[1], "see-test") == 0) {
            return seeTestSuite() ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        if (strcmp(argv[1], "eval-file") == 0 && argc >= 3) {
            long limit = argc >= 4 ? strtol(argv[3], NULL, 10) : 0;
            return evalFile(argv[2], limit);
        }
    }

    // Run UCI loop otherwise
    uciLoop();
}
