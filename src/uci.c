#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "uci.h"
#include "timeman.h"
#include "utils.h"
#include "makemove.h"
#include "move.h"
#include "perft.h"
#include "bench.h"
#include "eval.h"
#include "search.h"
#include "hashtable.h"

/* -------------------------------------------------------------------------- */
/*                                 UCI helpers                                */
/* -------------------------------------------------------------------------- */

// Initialises all the engine data to a valid initial state
void initEngine(Engine *engine) {
    // Setup board
    parseFen(&engine->board, START_FEN);

    // Setup engine state
    memset(&engine->pv, 0, sizeof(PV));
    memset(&engine->limits, 0, sizeof(SearchLimits));
    memset(&engine->searchStats, 0, sizeof(SearchInfo));
    engine->searchState = SEARCH_STOPPED;

    // Clear hash table
    clearHashTable();
}

// Converts a string to a move.
// Needs board to configure flags correctly. This was put here instead of move.c
// due to a circular dependency :/
Move stringToMove(char *string, Board *board) {
    // Get from and to squares from string
    int from = stringToSquare(string);
    int to = stringToSquare(string + 2);

    int pieceMoved = board->squares[from];
    int pieceCaptured = board->squares[to];
    int flag = QUIET_FLAG;

    // Promotion
    if (string[4] == 'q')
        flag |= QUEEN_PROMO_FLAG;
    else if (string[4] == 'r')
        flag |= ROOK_PROMO_FLAG;
    else if (string[4] == 'n')
        flag |= KNIGHT_PROMO_FLAG;
    else if (string[4] == 'b')
        flag |= BISHOP_PROMO_FLAG;

    // Capture
    if (pieceCaptured != EMPTY)
        flag |= CAPTURE_FLAG;

    // Castling
    if (pieceMoved == KING) {
        if (from == E1) {
            if (to == G1)
                flag = CASTLE_FLAG;
            else if (to == C1)
                flag = CASTLE_FLAG;
        } else if (from == E8) {
            if (to == G8)
                flag = CASTLE_FLAG;
            else if (to == C8)
                flag = CASTLE_FLAG;
        }
    }

    // En passant, check if pawn moves to epSquare diagonally.
    if (pieceMoved == PAWN && to == board->epSquare && fileOf(from) != fileOf(to)) {
        flag = EP_FLAG;
    }

    return ConstructMove(from, to, flag);
}

/* -------------------------------------------------------------------------- */
/*                            UCI Command Handlers                            */
/* -------------------------------------------------------------------------- */

// Identifies our engine, and its options to the GUI
void handleUci() {
    // Send Engine info
    printf("id name %s %s\n", NAME, VERSION);
    printf("id author %s\n", AUTHOR);

    // Send available options
    printf("option name Hash type spin default %d min %d max %d\n", HASH_SIZE_DEFAULT, HASH_SIZE_MIN, HASH_SIZE_MAX);
    puts("option name Clear Hash type button");
    puts("option name Threads type spin default 1 min 1 max 1");

    puts("uciok");
}

// Allows the GUI to configure options in our engine
void handleSetOption(char *input) {
    int hashSizeMB = -1;
    if (strncmp(input, "setoption name Hash value ", 26) == 0) {
        // Hash size option
        sscanf(input, "setoption name Hash value %d", &hashSizeMB);

        // Make sure the hash size is in limits
        if (hashSizeMB < HASH_SIZE_MIN) {
            printf("Hash size was too small, defaulting to %d\n", HASH_SIZE_DEFAULT);
            hashSizeMB = HASH_SIZE_DEFAULT;
        } else if (hashSizeMB > HASH_SIZE_MAX) {
            printf("Hash size was too big, defaulting to %d\n", HASH_SIZE_DEFAULT);
            hashSizeMB = HASH_SIZE_DEFAULT;
        }

        initHashTable(hashSizeMB);

    } else if (strcmp(input, "setoption name Clear Hash") == 0) {
        // Hash clear option
        puts("Hash table cleared.");
        clearHashTable();
    }
}

// New game command, sent before next search.
void handleUciNewGame(Engine *engine) {
    parseFen(&engine->board, START_FEN);
    clearHashTable();
    puts("readyok");
}

// Sets up the internal board to a state given
void handlePosition(Engine *engine, char *input) {
    // Set the position to either START_FEN or a provided FEN
    // Then plays the further moves specified on that position
    Board *board = &engine->board;
    char *token = strtok(input, " ");

    // Move pointer past position
    if (strcmp(token, "position") == 0) {
        token = strtok(NULL, " ");
    }

    if (strcmp(token, "startpos") == 0) {
        // Parse startpos
        parseFen(board, START_FEN);
        token = strtok(NULL, " "); // to 'moves' or NULL
    } else if (strcmp(token, "fen") == 0) {
        // Parse fen
        char fen[FEN_BUFFER_SIZE];
        fen[0] = '\0';
        token = strtok(NULL, " "); // to actual FEN part
        while (token != NULL && strcmp(token, "moves") != 0) {
            strcat(fen, token);
            strcat(fen, " ");
            token = strtok(NULL, " ");
        }
        parseFen(board, fen);
    }

    // Parse moves
    if (token != NULL && strcmp(token, "moves") == 0) {
        token = strtok(NULL, " ");
        while (token != NULL) {
            if (makeMove(board, stringToMove(token, board)) == 0) {
                // Move was illegal, or my code was wrong..
                printf("Illegal move found at: %s\n", token);
                exit(EXIT_FAILURE);
            }
            // Advance to next move
            token = strtok(NULL, " ");
        }
    }
}

// Start searching from this state
void handleGo(Engine *engine, char *input) {
    char *index;
    SearchLimits limits;

    // Default search type is infinite
    limits.depth = MAX_DEPTH - 1;
    limits.nodes = -1;
    limits.searchType = LIMIT_INFINITE;
    limits.searchStartTime = getTime();

    // Time control
    int moveTime = -1;
    int wtime = -1, btime = -1;
    int winc = 0, binc = 0;
    int movesToGo = -1;

    // Parse UCI go parameters
    index = strstr(input, "wtime");
    if (index) wtime = atoi(index + 6);
    
    index = strstr(input, "btime");
    if (index) btime = atoi(index + 6);
    
    index = strstr(input, "winc");
    if (index) winc = atoi(index + 5);
    
    index = strstr(input, "binc");
    if (index) binc = atoi(index + 5);
    
    index = strstr(input, "movestogo");
    if (index) movesToGo = atoi(index + 10);

    index = strstr(input, "depth");
    if (index) {
        limits.depth = atoi(index + 6);
        if (limits.depth <= 0) limits.depth = 1;
        if (limits.depth > MAX_DEPTH) limits.depth = MAX_DEPTH;
        limits.searchType = LIMIT_DEPTH;
    }

    index = strstr(input, "movetime");
    if (index) {
        moveTime = atoi(index + 9) - 50;
        limits.searchType = LIMIT_TIME;
    }

    index = strstr(input, "nodes");
    if (index) {
        limits.nodes = strtoull(index + 6, NULL, 10);
        limits.searchType = LIMIT_NODES;
    }

    if (strstr(input, "infinite")) {
        limits.searchType = LIMIT_INFINITE;
    }

    // If wtime or btime, then we're in a time limited search.
    if (wtime > 0 || btime > 0) limits.searchType = LIMIT_TIME;

    // Calculate time to search if time control is given
    if (limits.searchType == LIMIT_TIME && moveTime == -1) {
        int timeLeft = (engine->board.side == WHITE) ? wtime : btime;
        int increment = (engine->board.side == WHITE) ? winc : binc;

        // Set time limits
        calculateTimeManagement(&limits, timeLeft, increment, movesToGo);

        // printf("Soft bound time: %d ms\n", limits.softBoundTime - getTime());
        // printf("Hard bound time: %d ms\n", limits.hardBoundTime - getTime());
    } else if (limits.searchType == LIMIT_TIME) {
        // Set both hard bound and soft bound to movetime
        limits.hardBoundTime = getTime() + moveTime;
        limits.softBoundTime = getTime() + moveTime;
    }

    // Start the search within the given limits
    initSearch(engine, limits);
    Move bestMove = iterativeDeepening(engine);

    // Print the result of the search
    printf("bestmove %s\n", moveToString(bestMove));
    printf("Hash table occupied: %.2f%%\n", occupiedHashEntries());
}

// Clean up before exiting
void handleQuit() {
    // Free hash table
    cleanUpHashTable();
}

// Start perft from this position, defaults to 4
void handlePerft(Engine *engine, char *input) {
    int depth = 4;

    printBoard(&engine->board);
    if (strstr(input, "perft divide")) {
        // Provides a breakdown of the nodes after each move from this position.
        sscanf(input, "perft divide %d", &depth);
        perftDivide(&engine->board, depth);
    } else if (strstr(input, "perft test")) {
        // Runs a perft test suite on a set of positions
        perftSuite();
    } else {
        // Runs perft, and prints statistics like speed and time taken.
        sscanf(input, "perft %d", &depth);
        perftBench(&engine->board, depth);
    }
}

/* -------------------------------------------------------------------------- */
/*                                  UCI Loop                                  */
/* -------------------------------------------------------------------------- */

void uciLoop() {
    char input[INPUT_BUFFER_SIZE];
    
    // Use unbuffered I/O
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    // Initialise the engine data
    Engine engine;
    initEngine(&engine);
    
    // Handle commands
    while (true) {
        memset(input, 0, sizeof(input));

        // Take input line
        if (!fgets(input, INPUT_BUFFER_SIZE, stdin)) {
            // Quit on stdin closing
            handleQuit();
            break; 
        }

        // Strip newline
        input[strcspn(input, "\n")] = 0;
        
        /* UCI commands */
        if (strcmp(input, "uci") == 0) {
            handleUci();

        } else if (strcmp(input, "isready") == 0) {
            puts("readyok");

        } else if (strcmp(input, "ucinewgame") == 0) {
            handleUciNewGame(&engine);

        } else if (strncmp(input, "position", 8) == 0) {
            handlePosition(&engine, input);

        } else if (strncmp(input, "go", 2) == 0) {
            handleGo(&engine, input);

        } else if (strcmp(input, "quit") == 0) {
            handleQuit();
            break;
        } else if (strncmp(input, "setoption", 9) == 0) {
            handleSetOption(input);

        /* Custom commands */
        } else if (strncmp(input, "perft", 5) == 0) {
            handlePerft(&engine, input);
        } else if (strcmp(input, "bench") == 0) {
            // Run OpenBench benchmark
            bench();
        } else if (strcmp(input, "print") == 0) {
            printBoard(&engine.board);
        } else if (strcmp(input, "eval") == 0) {
            printEvaluation(&engine.board);
        }

        /* Unknown command */
        else {
            printf("Unknown command: '%s'\n", input);
        }
    }
}
