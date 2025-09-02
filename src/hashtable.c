#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hashtable.h"
#include "bitboards.h"
#include "board.h"
#include "move.h"
#include "search.h"


// Global variable :skull:
HashTable hashTable;


void clearHashTable() {
    // Loop through the hash entries, setting all the values to empty
    for (int i = 0; i < hashTable.count; i++) {
        // Clear entry
        hashTable.entries[i].hashKey = 0ULL;
        hashTable.entries[i].bestMove = NO_MOVE;
        hashTable.entries[i].depth = 0;
        hashTable.entries[i].score = 0;
        hashTable.entries[i].flag = 0;
    }
}

void freeHashTable() {
    free(hashTable.entries);
}

// Returns the percentage of hash table occupancy
double occupiedHashEntries() {
    int occupied = 0;
    for (int i = 0; i < hashTable.count; i++) {
        if (hashTable.entries[i].hashKey != 0ULL)
            occupied++;
    }
    return ((double)occupied / (double)hashTable.count) * 100.0;
}

// Initialises hash table to certain size in MB
void initHashTable(int sizeMB) {
    // Calculate how many hash entries to match the size
    uint64_t size = sizeMB * 0x100000;
    hashTable.count = size / sizeof(HashEntry);
    hashTable.count -= 2; // for safety (inherited from VICE)

    // Free hash table
    free(hashTable.entries);

    // Allocate and clear the table
    hashTable.entries = (HashEntry *)malloc(hashTable.count * sizeof(HashEntry));

    // Check if allocation failed
    if (hashTable.entries == NULL) {
        puts("Hash allocation failed.");
        puts("Check if you have enough memory");
        exit(EXIT_FAILURE);
    }

    clearHashTable();

    printf("Hash size set to %d MB\n", sizeMB);
    printf("Number of hash entries: %lu\n", hashTable.count);
}

void hashTableStore(U64 hash, Move bestMove, int depth, int score, int flag) {
    // Calculate hash index and retrieve corresponding bucket
    int index = hash % hashTable.count;
    HashEntry *entry = &hashTable.entries[index];

    entry->hashKey = hash;
    entry->bestMove = bestMove;
    entry->depth = depth;
    entry->score = score;
    entry->flag = flag;
}

int hashTableProbe(U64 hash, Move *hashMove, int *depth, int *score, int *flag) {
    // Calculate hash index and retrieve corresponding bucket
    int index = hash % hashTable.count;
    HashEntry *entry = &hashTable.entries[index];

    // Check the first entry
    if (entry->hashKey == hash) {
        // Copy data over
        *hashMove = entry->bestMove;
        *depth = entry->depth;
        *score = entry->score;
        *flag = entry->flag;

        return PROBE_SUCCESS;
    }

    return PROBE_FAIL;
}
