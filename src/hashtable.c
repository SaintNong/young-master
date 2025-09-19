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

/* -------------------------------------------------------------------------- */
/*                         Hash table helper functions                        */
/* -------------------------------------------------------------------------- */

/**
 * Mate scores are relative to the current ply, but when stored and retrieved
 * from the hash table, they need to be adjusted for the ply difference. If this
 * is not done, then the engine distinguish faster/slower mates when retrieving
 * scores from hash table.
 */

// If mate score, convert the score from root-relative to node-relative.
static int toHashScore(int score, int ply) {
    return (score >= MATE_BOUND) ? score - ply :
           (score <= -MATE_BOUND) ? score + ply : score;
}

// If mate score, convert the score from node-relative to root-relative.
static int fromHashScore(int score, int ply) {
    return (score >= MATE_BOUND) ? score + ply :
           (score <= -MATE_BOUND) ? score - ply : score;
}

// Clears every entry of the hash table
void clearHashTable() {
    // Loop through the hash entries, setting all the values to empty
    for (U64 i = 0; i < hashTable.count; i++) {
        // Clear entry
        hashTable.entries[i].hashKey = 0ULL;
        hashTable.entries[i].bestMove = NO_MOVE;
        hashTable.entries[i].depth = 0;
        hashTable.entries[i].score = 0;
        hashTable.entries[i].flag = 0;
    }
}

// Cleans up the heap allocated memory the hash table uses.
void cleanUpHashTable() {
    free(hashTable.entries);
}

// Returns the percentage of hash table occupancy
double occupiedHashEntries() {
    int occupied = 0;
    for (U64 i = 0; i < hashTable.count; i++) {
        if (hashTable.entries[i].hashKey != 0ULL)
            occupied++;
    }
    return ((double)occupied / (double)hashTable.count) * 100.0;
}

// Initialises hash table to certain size in MB
void initHashTable(int sizeMB) {
    // Calculate how many hash entries to match the size
    uint64_t size = sizeMB * BYTES_PER_MB;
    hashTable.count = size / sizeof(HashEntry);

    // Free the old hash table before reallocating
    free(hashTable.entries);

    // Allocate and clear the table
    hashTable.entries = (HashEntry *)malloc(hashTable.count * sizeof(HashEntry));

    // Check if allocation failed
    if (hashTable.entries == NULL) {
        puts("Hash allocation failed.");
        puts("Check if you have enough memory?");
        exit(EXIT_FAILURE);
    }

    clearHashTable();

    printf("info string Hash size: %d MB\n", sizeMB);
    // printf("Number of hash entries: %lu\n", hashTable.count);
}


// Stores given information into the hash table.
void hashTableStore(U64 hash, int ply, Move bestMove, int depth, int score, int flag) {
    // Calculate hash index and retrieve corresponding entry
    int index = hash % hashTable.count;
    HashEntry *entry = &hashTable.entries[index];

    // Don't replace the hash move if it's the same position and we don't have one.
    if (bestMove != NO_MOVE || entry->hashKey != hash) {
        entry->bestMove = bestMove;
    }

    // Always replace strategy
    entry->hashKey = hash;
    entry->depth = depth;
    entry->score = toHashScore(score, ply);
    entry->flag = flag;
}

// Probes hash table for information about the current position
int hashTableProbe(U64 hash, int ply, Move *hashMove, int *depth, int *score, int *flag) {
    // Calculate hash index and retrieve corresponding entry
    int index = hash % hashTable.count;
    HashEntry *entry = &hashTable.entries[index];

    // Check the first entry
    if (entry->hashKey == hash) {
        // Copy data over
        *hashMove = entry->bestMove;
        *depth = entry->depth;
        *score = fromHashScore(entry->score, ply);
        *flag = entry->flag;

        return PROBE_SUCCESS;
    }

    return PROBE_FAIL;
}

/**
 * Probes hash table for just the best move and score
 */
Move probeHashMove(U64 hash, int *score) {
    // Calculate hash index and retrieve corresponding entry
    int index = hash % hashTable.count;
    HashEntry *entry = &hashTable.entries[index];

    if (entry->hashKey == hash) {
        // Return the move if the entry exists
        *score = fromHashScore(entry->score, 0);
        return entry->bestMove;
    }

    return NO_MOVE;
}
