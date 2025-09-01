#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "uci.h"
#include "utils.h"
#include "makemove.h"
#include "move.h"
#include "bench.h"
#include "eval.h"
#include "search.h"

/* -------------------------------------------------------------------------- */
/*                                 UCI helpers                                */
/* -------------------------------------------------------------------------- */

// Initialises all the engine data to a valid initial state
void initEngine(Engine *engine) {
    parseFen(&engine->board, START_FEN);

    memset(&engine->pv, 0, sizeof(PV));
    memset(&engine->limits, 0, sizeof(SearchLimits));
    memset(&engine->searchStats, 0, sizeof(SearchInfo));
    engine->searchState = SEARCH_STOPPED;
}

// Converts a string to a move.
// Needs board to configure flags correctly.
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
    else if (pieceCaptured != EMPTY)
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

    // En passant
    if (pieceMoved == PAWN && string[0] != string[2] && pieceCaptured == EMPTY)
        flag = EP_FLAG;

    return ConstructMove(from, to, flag);
}

// Simple time management function to determine how long to search
int calculateTimeToThink(int timeLeft, int increment, int movesToGo) {
    if (movesToGo < 0) {
        movesToGo = 40;
    }
    return (timeLeft / (movesToGo * 1.5)) + (increment / 2);
}

/* -------------------------------------------------------------------------- */
/*                            UCI Command Handlers                            */
/* -------------------------------------------------------------------------- */

// Identifies our engine, and its options to the GUI
void handleUci() {
    printf("id name %s %s\n", NAME, VERSION);
    printf("id author %s\n", AUTHOR);
    puts("uciok");
}

// New game command, sent before next search.
void handleUciNewGame(Engine *engine) {
    parseFen(&engine->board, START_FEN);
    puts("readyok");
}

// Sets up the internal board to a state given
void handlePosition(Engine *engine, char *input) {
    // Set the position to either START_FEN or a provided FEN
    // Then plays the further moves specified on that position
    Board *board = &engine->board;
    char *token = strtok(input, " ");

    if (strcmp(token, "position") == 0) {
        token = strtok(NULL, " ");
    }

    if (strcmp(token, "startpos") == 0) {
        parseFen(board, START_FEN);
        token = strtok(NULL, " "); // to 'moves' or NULL
    } else if (strcmp(token, "fen") == 0) {
        char fen[100];
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
                printf("Move parsing error at: %s\n", token);
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
    limits.depth = MAX_DEPTH;
    limits.nodes = -1;
    limits.searchType = LIMIT_INFINITE;

    // Time control
    int timeToSearch = -1;
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
        limits.searchType = LIMIT_DEPTH;
    }

    index = strstr(input, "movetime");
    if (index) {
        timeToSearch = atoi(index + 9);
        limits.searchType = LIMIT_TIME * 0.9;
    }

    index = strstr(input, "nodes");
    if (index) {
        limits.nodes = strtoull(index + 6, NULL, 10);
        limits.searchType = LIMIT_NODES;
    }

    if (strstr(input, "infinite")) {
        limits.searchType = LIMIT_INFINITE;
    }

    if (wtime > 0 || btime > 0) limits.searchType = LIMIT_TIME;

    // Calculate time to search if time control is given
    if (limits.searchType == LIMIT_TIME && timeToSearch == -1) {
        int timeLeft = (engine->board.side == WHITE) ? wtime : btime;
        int increment = (engine->board.side == WHITE) ? winc : binc;
        timeToSearch = calculateTimeToThink(
            timeLeft,
            increment,
            movesToGo
        );

        printf("Time budgeted for search: %d ms\n", timeToSearch);
    }
    limits.searchStopTime = getTime() + timeToSearch;


    // Start the search
    initSearch(engine);
    engine->limits = limits;
    Move bestMove = iterativeDeepening(engine);

    printf("bestmove %s\n", moveToString(bestMove));
}

// Clean up before exiting
void handleQuit(Engine *engine) {

}

// Start perft from this position, defaults to 4
void handlePerft(Engine *engine, char *input) {
    int depth = 4;
    sscanf(input, "perft %d", &depth);

    printBoard(&engine->board);
    bench(&engine->board, depth);
}



/* -------------------------------------------------------------------------- */
/*                                  UCI Loop                                  */
/* -------------------------------------------------------------------------- */

void uciLoop() {
    char input[INPUT_BUFFER_SIZE];
    
    // Use unbuffered I/O
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    // Initialise the engine data
    Engine engine;
    initEngine(&engine);
    
    // Handle commands
    while (true) {
        memset(input, 0, sizeof(input));

        // Take input line
        if (!fgets(input, INPUT_BUFFER_SIZE, stdin))
            continue;

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
            handleQuit(&engine);
            break;
        }

        /* Custom commands */
        else if (strncmp(input, "perft", 5) == 0) {
            handlePerft(&engine, input);
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
