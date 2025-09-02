#pragma once

#include "uci.h"

// Hash flags
enum {
  // Entry does not exist
  BOUND_NONE = 0,

  // => All moves were searched and we failed to find a better move
  BOUND_UPPER = 1,

  // => A beta cut-off happened and we stopped searching early
  BOUND_LOWER = 2,

  // => All moves were searched and one was found to increase alpha.
  BOUND_EXACT = 3,

};

// Probing flags
enum { PROBE_FAIL, PROBE_SUCCESS };

// Hash entry
typedef struct {
  U64 hashKey;
  Move bestMove;
  int16_t depth;
  int16_t score;
  int16_t flag;
} HashEntry;

typedef struct {
  HashEntry *entries;
  uint64_t count;
} HashTable;

// Hash table functions
void initHashTable(int sizeMB);
void freeHashTable();
void clearHashTable();
double occupiedHashEntries();

// For use in game
void hashTableStore(U64 hash, Move bestMove, int depth, int score, int flag);
int hashTableProbe(U64 hash, Move *hashMove, int *depth, int *score, int *flag);
void updateHashAge();



