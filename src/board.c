#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "board.h"
#include "zobrist.h"
#include "bitboards.h"
#include "magicmoves.h"
#include "utils.h"

/* -------------------------------------------------------------------------- */
/*                               Square helpers                               */
/* -------------------------------------------------------------------------- */

// Square index from file and rank indices
int squareFrom(int file, int rank) { return rank * 8 + file; }

// Get the rank of a square index
int rankOf(int sq) { return sq / 8; }

// Get the file of a square index
int fileOf(int sq) { return sq % 8; }

// Check if the file and rank index is a valid square in a chess board
bool fileRankInBoard(int file, int rank) {
    if (file < 0 || file >= 8) {
        return false;
    }
    if (rank < 0 || rank >= 8) {
        return false;
    }
    return true;
}

// Converts a square index to coordinates string
void squareToString(int square, char *coords) {
    int file = fileOf(square);
    int rank = rankOf(square);
    coords[0] = 'a' + file;
    coords[1] = '1' + rank;
    coords[2] = '\0';
}

// Converts a string notating a square to the square index
int stringToSquare(char *string) {
    return squareFrom(string[0] - 'a', string[1] - '1');
}

/* -------------------------------------------------------------------------- */
/*                                Board Actions                               */
/* -------------------------------------------------------------------------- */

// Sets a piece on the board at the square
void setPiece(Board *board, int color, int piece, int sq) {
    assert(board->squares[sq] == EMPTY);      // Square is empty
    assert(piece >= PAWN && piece <= KING);   // Valid piece
    assert(color == WHITE || color == BLACK); // Valid color
    assert(sq >= A1 && sq <= H8);             // Valid square

    // Set piece on pieces bitboard, colors[color] and colors[BOTH]
    setBit(&board->pieces[piece], sq);
    setBit(&board->colors[color], sq);
    setBit(&board->colors[BOTH], sq);

    // Set piece on mailbox board
    board->squares[sq] = piece;

    // Update board hash
    board->hash ^= PieceKeys[toPiece(piece, color)][sq];
}

// Clears the piece from the board on the square specified
void clearPiece(Board *board, int color, int sq) {
    int piece = board->squares[sq];

    assert(piece >= PAWN && piece <= KING);   // Valid piece
    assert(color == WHITE || color == BLACK); // Valid color
    assert(sq >= A1 && sq <= H8);             // Valid square

    // Clear piece on mailbox board
    board->squares[sq] = EMPTY;

    // Clear piece from bitboards
    clearBit(&board->pieces[piece], sq);
    clearBit(&board->colors[color], sq);
    clearBit(&board->colors[BOTH], sq);

    // Update board hash
    board->hash ^= PieceKeys[toPiece(piece, color)][sq];
}

// Moves piece from one square to on board
void movePiece(Board *board, int from, int to, int color) {
    // Clear the from square
    int piece = board->squares[from];

    assert(board->squares[to] == EMPTY);      // to sq is empty
    assert(piece >= PAWN && piece <= KING);   // Valid piece
    assert(color == WHITE || color == BLACK); // Valid color
    assert(from >= A1 && from <= H8);         // Valid from square
    assert(to >= A1 && to <= H8);             // Valid to square

    board->squares[from] = EMPTY;

    // Clear piece from bitboards
    clearBit(&board->pieces[piece], from);
    clearBit(&board->colors[color], from);
    clearBit(&board->colors[BOTH], from);

    // Set piece on pieces bitboard, colors[color] and colors[BOTH]
    setBit(&board->pieces[piece], to);
    setBit(&board->colors[color], to);
    setBit(&board->colors[BOTH], to);

    // Set on mailbox board
    board->squares[to] = piece;

    // Update board hash
    board->hash ^= PieceKeys[toPiece(piece, color)][from];
    board->hash ^= PieceKeys[toPiece(piece, color)][to];
}

// Clears the board to an empty state
void clearBoard(Board *board) {
    // Clear board pieces
    memset(board->colors, 0ULL, sizeof(board->colors));
    memset(board->pieces, 0ULL, sizeof(board->pieces));

    for (int sq = 0; sq < 64; sq++) {
        board->squares[sq] = EMPTY;
    }

    // Clear board variables
    board->side = BOTH;
    board->hash = 0ULL;
    board->epSquare = NO_SQ;
    board->fiftyMove = 0;
    board->castlePerm = 0;
    board->hisPly = 0;

    // Clear history
    for (int i = 0; i < MAX_MOVES; i++) {
        Undo *undo = &board->history[i];
        undo->hash = 0ULL;
        undo->castlePerm = 0;
        undo->epSquare = NO_SQ;
        undo->fiftyMove = 0;
        undo->movedPiece = NO_PIECE;
        undo->capturedPiece = NO_PIECE;
    }
}

/* -------------------------------------------------------------------------- */
/*                              Board Information                             */
/* -------------------------------------------------------------------------- */

// Returns true if a certain square is attacked
int isSquareAttacked(Board *board, int color, int square) {
    int enemy = !color;
    U64 occ = board->colors[BOTH];

    U64 enemyPawns = board->colors[enemy] & board->pieces[PAWN];
    U64 enemyKnights = board->colors[enemy] & board->pieces[KNIGHT];
    U64 enemyBishops = board->colors[enemy] & (
        board->pieces[BISHOP] | board->pieces[QUEEN]);
    U64 enemyRooks = board->colors[enemy] & (
        board->pieces[ROOK] | board->pieces[QUEEN]);
    U64 enemyKings = board->colors[enemy] & board->pieces[KING];

    return (pawnAttacks(color, square) & enemyPawns)
        || (knightAttacks(square) & enemyKnights)
        || (enemyBishops && (Bmagic(square, occ) & enemyBishops))
        || (enemyRooks && (Rmagic(square, occ) & enemyRooks))
        || (kingAttacks(square) & enemyKings);
}

// Returns true if the current side to move is in check.
int boardIsInCheck(Board *board) {
    return isSquareAttacked(
        board,
        board->side,
        getlsb(board->pieces[KING] & board->colors[board->side])
    );
}

// All attackers of a certain square
U64 allAttackersToSquare(Board *board, U64 occupied, int sq) {
    return (pawnAttacks(WHITE, sq) & board->colors[BLACK] & board->pieces[PAWN])
        |  (pawnAttacks(BLACK, sq) & board->colors[WHITE] & board->pieces[PAWN])
        |  (knightAttacks(sq) & board->pieces[KNIGHT])
        |  (Bmagic(sq, occupied) & (board->pieces[BISHOP] | board->pieces[QUEEN]))
        |  (Rmagic(sq, occupied) & (board->pieces[ROOK] | board->pieces[QUEEN]))
        |  (kingAttacks(sq) & board->pieces[KING]);
}

// Wrapper for allAttackersToSquare() for use in double check detection
U64 attackersToKingSquare(Board *board) {
    int kingsq = getlsb(board->colors[board->side] & board->pieces[KING]);
    U64 occupied = board->colors[BOTH];
    return allAttackersToSquare(board, occupied, kingsq) &
        board->colors[!board->side];
}

// Check if zugzwang is unlikely in the current position.
// If we have any non-pawn pieces, we can probably avoid zugswang.
bool nullMoveIsBad(Board *board) {
    U64 us = board->colors[board->side];
    U64 kings = board->pieces[KING];
    U64 pawns = board->pieces[PAWN];
    
    return (us & (kings | pawns)) == us;
}

// Detects if board has insufficient material to mate
bool insufficientMaterial(Board *board) {
    // KvK, KvN, KvNN, KvB.
    return !(board->pieces[PAWN] | board->pieces[ROOK] | board->pieces[QUEEN])
        && (!multipleBits(board->colors[WHITE]) || !multipleBits(board->colors[BLACK]))
        && (!multipleBits(board->pieces[KNIGHT] | board->pieces[BISHOP])
        || (!board->pieces[BISHOP] && popCount(board->pieces[KNIGHT]) <= 2));
}

// Checks if the position is a draw by three-fold repetition
bool isRepetition(Board *board, int ply) {
    // Index in history where current search root is located.
    int rootIndex = board->hisPly - ply;

    /**
     * Go backwards in history, twice at a time to only check positions from our
     * side to move. We tally up positions where the hash matches ours.
     */
    int repetitions = 0;
    for (int i = board->hisPly - 2; i >= 0; i -= 2) {
        // Repetitions can't ever happen before an irreversible move was made.
        if (i < board->hisPly - board->fiftyMove) break;

        // Check matching hash
        if (board->history[i].hash == board->hash) {
            // Count the repetition
            repetitions++;

            /**
             * If the repetition occurred after the root node, then the current
             * player (us) is forcing a draw.
             */
            if (i > rootIndex)
                return true;

            /**
             * Otherwise, if full three fold repetition occurs in-part before
             * the root node then that is a draw too.
             */
            if (repetitions >= 2)
                return true;
        }
    }
    return false;
}

// Detects draws on the board
bool isDraw(Board *board, int ply) {
    /**
     * There are 3 types of draws:
     * 1. Fifty move
     * 2. Three fold repetition
     * 3. Insufficient material
     */

    // Type 1. Fifty move
    if (board->fiftyMove >= 100)
        return true;

    // Type 2. Three fold
    if (isRepetition(board, ply))
        return true;

    // Type 3. Insufficient material
    if (insufficientMaterial(board))
        return true;

    return false;
}

/* -------------------------------------------------------------------------- */
/*                                  Board IO                                  */
/* -------------------------------------------------------------------------- */

// Displays the given board on the terminal
void printBoard(Board *board) {
    const char asciiPieces[12] = "PNBRQKpnbrqk";
    int sq;
    int isEmpty;

    printf("\n *  a b c d e f g h  *\n\n");

    // print pieces
    for (int rank = 7; rank >= 0; rank--) {
        printf(" %d ", rank + 1);

        for (int file = 0; file < 8; file++) {
            sq = squareFrom(file, rank);

            isEmpty = 1;
            for (int piece = PAWN; piece < NB_PIECES; piece++) {
                if (testBit(board->pieces[piece], sq)) {
                    assert(board->squares[sq] == piece);

                    isEmpty = 0;
                    if (testBit(board->colors[WHITE], sq)) {
                        printf(BLU " %c", asciiPieces[toPiece(piece, WHITE)]);
                    } else if (testBit(board->colors[BLACK], sq)) {
                        printf(RED " %c", asciiPieces[toPiece(piece, BLACK)]);
                    } else {
                        puts("board is corrupted\n");
                        return;
                  }
                }
            }

            if (isEmpty) {
                printf("  ");
                assert(board->squares[sq] == EMPTY);
            }
        }
        // Rank number
        printf(CRESET "  %d", rank + 1);

        // print stats next to board
        if (rank == 7) {
            printf("    | %s to move", (board->side == WHITE) ? ( BLU "White" CRESET ) : ( RED "Black" CRESET));
        } else if (rank == 6) {
            printf("    |");
        } else if (rank == 5) {
            printf("    | Castle      %s%s%s %s%s",
                (board->castlePerm == 0) ? "n/a" : "",
                (board->castlePerm & CASTLE_WK) ? BLU "K" CRESET : "",
                (board->castlePerm & CASTLE_WQ) ? BLU "Q" CRESET : "",
                (board->castlePerm & CASTLE_BK) ? RED "k" CRESET : "",
                (board->castlePerm & CASTLE_BQ) ? RED "q" CRESET : "");
        } else if (rank == 4) {
            char epString[3];
            if (board->epSquare != NO_SQ) {
                squareToString(board->epSquare, epString);
                printf("    | EP square   %s", epString);
            } else {
                printf("    | EP square   n/a");
            }
        } else if (rank == 3) {
            printf("    | 50 move     %d", board->fiftyMove);
        } else if (rank == 2) {
            printf("    | Hash        " CYN "%" PRIx64 CRESET, board->hash);
        }

        // Newline
        puts("");
    }
    printf("\n *  a b c d e f g h  *\n\n");
}

// Sets a provided board to the provided FEN
void parseFen(Board *board, char *fen) {
    int sq;
    int color;
    int piece;

    clearBoard(board);

    // Place pieces
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            sq = squareFrom(file, rank);

            // if the character in the fen is a '/', the next rank is reached
            if (*fen == '/') {
                fen++;
            }

            // if the character is a number, offset the file by that number
            if (*fen >= '1' && *fen <= '8') {
                int offset = *fen - '0';

                // adjust by -1 to account for the loop increment
                file += offset - 1;
            }

            // if the character is alphabetic, it is a piece
            if (isalpha(*fen)) {
                if (isupper(*fen)) {
                    color = WHITE;
                } else {
                    color = BLACK;
                }

                switch (tolower(*fen)) {
                case 'p':
                    piece = PAWN;
                    break;
                case 'n':
                    piece = KNIGHT;
                    break;
                case 'b':
                    piece = BISHOP;
                    break;
                case 'r':
                    piece = ROOK;
                    break;
                case 'q':
                    piece = QUEEN;
                    break;
                case 'k':
                    piece = KING;
                    break;
                default:
                    printf("FEN parsing error while placing pieces\n");
                    exit(EXIT_FAILURE);
                    break;
                }

                setPiece(board, color, piece, sq);
            }

            fen++;
        }
    }

    // Set side to move
    fen++;
    if (*fen == 'w') {
        board->side = WHITE;
    } else if (*fen == 'b') {
        board->side = BLACK;
    } else {
        puts("FEN parsing error while setting side to move");
        exit(EXIT_FAILURE);
    }
    fen++;
    fen++;

    // Set castle rights
    board->castlePerm = 0;
    while (*fen != ' ') {
        switch (*fen) {
        case 'K':
            board->castlePerm |= CASTLE_WK;
            break;
        case 'Q':
            board->castlePerm |= CASTLE_WQ;
            break;
        case 'k':
            board->castlePerm |= CASTLE_BK;
            break;
        case 'q':
            board->castlePerm |= CASTLE_BQ;
            break;
        case '-':
            break;
        default:
            puts("FEN parsing error in castling rights");
            exit(EXIT_FAILURE);
            break;
        }
        fen++;
    }
    fen++;

    // set en passant square
    if (*fen != '-') {
        int file = fen[0] - 'a';
        int rank = fen[1] - '1';
        board->epSquare = squareFrom(file, rank);
    }
    fen += 2;

    // Set fifty move and hisply counter
    board->fiftyMove = strtol(fen, &fen, 10);
    fen++;

    // We ignore the full move counter because hisPly is reset whenever we reset the fen
    // int fullMoves = strtol(fen, &fen, 10);
    board->hisPly = 0;

    // Reset the Zobrist hash
    board->hash = generateHash(board);
    assert(board->hash == generateHash(board));
}

