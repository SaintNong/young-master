#include <stdio.h>
#include <stdbool.h>

#include "move.h"
#include "board.h"

// Converts a move to a string
char* moveToString(Move move) {
    if (move == NO_MOVE) {
        return "0000";
    }

    static char string[6];
    int from = MoveFrom(move);
    int to = MoveTo(move);

    squareToString(from, &string[0]);
    squareToString(to, &string[2]);
    
    if (IsPromotion(move)) {
        switch (MovePromotedPiece(move)) {
            case KNIGHT:
                string[4] = 'n';
                break;
            case BISHOP:
                string[4] = 'b';
                break;
            case ROOK:
                string[4] = 'r';
                break;
            case QUEEN:
                string[4] = 'q';
                break;
        }
        string[5] = '\0';
    } else {
        string[4] = '\0';
    }
    
    return string;
}

void printMove(Move move, bool includeNewLine) {
    char *moveStr = moveToString(move);
    printf("%s", moveStr);

    if (includeNewLine) {
        printf("\n");
    }
}
