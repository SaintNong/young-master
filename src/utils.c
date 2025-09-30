#include "utils.h"

// Returns time since epoch in milliseconds
int getTime() {
#if defined(_WIN32) || defined(_WIN64)
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

// Cross-platform function to check for user input
int pollUserInput() {
#if defined(_WIN32) || defined(_WIN64)
    /**
     * Thank you to the Code Monkey King, who used this code in BBC. He took it
     * from VICE by Richard Allbert, who grabbed it from somewhere else...
     * https://github.com/maksimKorzh/bbc/blob/master/src/bbc_nnue/bbc.c#L352
     */

    static int init = 0, pipe;
    static HANDLE inh;
    DWORD dw;

    if (!init)
    {
        init = 1;
        inh = GetStdHandle(STD_INPUT_HANDLE);
        pipe = !GetConsoleMode(inh, &dw);
        if (!pipe)
        {
            SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
            FlushConsoleInputBuffer(inh);
        }
    }
    
    if (pipe)
    {
        if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
        return dw;
    }
    
    else
    {
        GetNumberOfConsoleInputEvents(inh, &dw);
        return dw <= 1 ? 0 : dw;
    }
#else
    // https://talkchess.com/viewtopic.php?t=39870
    // Thank you to lucasart!
    fd_set readfds;
	struct timeval timeout;

	FD_ZERO(&readfds);
	FD_SET(0, &readfds);	// fd 0 is always stdin
	timeout.tv_sec = timeout.tv_usec = 0;
	
	select(1, &readfds, 0, 0, &timeout);
	return (FD_ISSET(0, &readfds));
#endif
}

// Check if user entered "stop" command
int checkUserStop() {
    if (!pollUserInput()) {
        return false;
    }

    char input[LINE_WIDTH];

    // Read the available input
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // Remove newline characters
        input[strcspn(input, "\n")] = '\0';
        
        if (strcmp(input, "stop") == 0) {
            return true;
        } else if (strcmp(input, "quit") == 0) {
            // Cleanup and exit instantly
            handleQuit();
            exit(EXIT_SUCCESS);
        }
    }
    
    return false;
}

// ANSI colour printing helpers
void printf_success(const char *format, ...) {
    va_list args;
    printf("%s", GRN);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("%s", CRESET);
}

void printf_fail(const char *format, ...) {
    va_list args;
    printf("%s", RED);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("%s", CRESET);
}


/**
 * Returns the number clamped between the lower and upper values
 */
int clamp(int x, int low, int high) {
  const int t = x < low ? low : x;
  return t > high ? high : t;
}
