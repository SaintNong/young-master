#pragma once

#include "uci.h"


// Sets the hard bound and soft bounds in the search limits.
void calculateTimeManagement(SearchLimits *limits, int timeLeft, int increment, int movesToGo);
bool timeSoftBoundReached(SearchLimits *limits);
