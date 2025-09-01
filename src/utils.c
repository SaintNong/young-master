#include "utils.h"
#include "bitboards.h"

#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>

// Returns time since epoch in milliseconds
int getTime() {
#ifdef WIN32
    return GetTickCount();
#else
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000 + t.tv_usec / 1000;
#endif
}

// XOR shift algorithm from Wikipedia
// https://en.wikipedia.org/wiki/Xorshift
U64 randomU64() {
    static U64 seed = 0xD9163F3DE9C71A8BULL;

    seed ^= seed >> 12;
    seed ^= seed << 21;
    seed ^= seed >> 27;

    return seed * 0x2545F4914F6CDD1DULL;
}
