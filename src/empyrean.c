#include <stdio.h>
#include <assert.h>

#include "uci.h"
#include "bitboards.h"
#include "magicmoves.h"
#include "movepicker.h"
#include "zobrist.h"
#include "hashtable.h"
#include "search.h"
#include "perft.h"
#include "eval.h"

void welcome() {
    printf("%s (%s) is suddenly interrupted during his deep seclusion.", NAME, VERSION);
    puts("\nA mere mortal like you dares to interrupt this venerable one's "
        "cultivation to challenge me to game of chess?!");
    puts("You are courting death! Insolent junior, prepare to have your foundation "
        "shattered and your meridians severed.");
    puts("You have eyes but you could not see Mount Tai!");
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