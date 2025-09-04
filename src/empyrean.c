#include <stdio.h>
#include <assert.h>

#include "uci.h"
#include "bitboards.h"
#include "magicmoves.h"
#include "movepicker.h"
#include "zobrist.h"
#include "hashtable.h"
#include "search.h"

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "bench.h"

void welcome() {
    printf("%s (%s) is suddenly interrupted during his deep seclusion.", NAME, VERSION);
    puts("\nA mere mortal like you dares to interrupt this venerable one's "
        "cultivation to challenge me to game of chess?!");
    puts("You are courting death! Insolent junior, prepare to have your foundation "
        "shattered and your meridians severed.");
}

void initialise() {
    // Movegen
    initmagicmoves();
    initAttackMasks();
    
    // Search
    initMvvLva();
    initZobristKeys();
    initHashTable(128);
    initLMRTable();
}

int main(void) {
    welcome();
    initialise();

    uciLoop();
}