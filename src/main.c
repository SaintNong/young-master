#include <stdio.h>
#include <assert.h>

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

#define NAME_VERSION_STRING WHT NAME " [" CYN VERSION WHT "]" CRESET
#include "string.h"

void welcome() {
    // Print versioning info
    // printf("Length %lu\n", strlen(NAME_VERSION_STRING));
    puts("╔═════════════════════════════════════╗");
    printf("║  %s        ║\n", NAME_VERSION_STRING);
    printf("║  "WHT "Compiled on: %s  ║\n" CRESET, __DATE__ " " __TIME__);
    puts("╚═════════════════════════════════════╝");

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
    initLMRTable();

    // Evaluation
    initEvaluation();
}

int main(void) {
    welcome();
    initialise();

    uciLoop();
}