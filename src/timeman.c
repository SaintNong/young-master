#include <stdbool.h>
#include "utils.h"
#include "timeman.h"

/**
 * Checks if the soft bound was reached, called before each iteration.
 */
bool timeSoftBoundReached(SearchLimits *limits) {
    // Soft bound is disabled on non time limited searches
    if (limits->searchType != LIMIT_TIME) return false;

    // Check if soft bound is reached
    if (getTime() > limits->softBoundTime) {
        return true;
    }
    return false;
}

/**
 * Very simple time management formula.
 * https://www.chessprogramming.org/Time_Management#Base_TM
 */
int calculateHardBound(int timeLeft, int increment, int movesToGo) {
    if (movesToGo < 0) {
        movesToGo = 20;
    }
    return (timeLeft / (movesToGo + 2)) + (increment / 2);
}


/**
 * Time management.
 * This function calculates how much time to allocate for the search based on how
 * much time we have left. This uses a hard bound and a soft bound, where the hard
 * bound is checked during the search function itself, and the soft bound breaks
 * the iterative deepening loop early if reached at the end of an iteration.
 * https://www.chessprogramming.org/Time_Management#Soft_Bound
 */
void calculateTimeManagement(SearchLimits *limits, int timeLeft, int increment, int movesToGo) {
    // Get hard and soft bounds
    int hardBound = calculateHardBound(timeLeft, increment, movesToGo);
    int softBound = 0.5 * hardBound;

    // Set hard time and soft time bounds (in ms since epoch)
    int currentTime = getTime();
    limits->hardBoundTime = currentTime + hardBound;
    limits->softBoundTime = currentTime + softBound;
    printf("%d\n", limits->softBoundTime);
}

