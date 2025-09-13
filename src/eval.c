#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "eval.h"
#include "board.h"
#include "bitboards.h"
#include "magicmoves.h"


/* -------------------------------------------------------------------------- */
/*                             Tapered Evaluation                             */
/* -------------------------------------------------------------------------- */

/**
 * The game phase increments for each piece on the board, weighted by its value.
 * The lower this value, the closer the board is to an endgame. This is used to
 * interpolate the evaluation score between the middlegame and endgame to prevent
 * evaluation discontinuity. This value starts at 24, and lowers to zero by the
 * time a pawn endgame is reached.
 * 
 * TODO: maybe do phase incrementally
 */
int getGamePhase(Board *board) {
    int phase = 0;

    // Loop through all pieces which affect game phase.
    for (int piece = KNIGHT; piece <= QUEEN; piece++) {
        // Increment the phase by how many pieces of that type is on the board.
        phase += popCount(board->pieces[piece]) * GAME_PHASE_INCREMENTS[piece];
    }
    
    // Clamp phase in case someone promotes early.
    if (phase > PHASE_MAX) phase = PHASE_MAX;
    return phase;
}


// Interpolates the middlegame and endgame scores based on the phase.
int taper(int score, int phase) {
    int scoreMG = ScoreMG(score);
    int scoreEG = ScoreEG(score);

    return (scoreMG * phase + scoreEG * (PHASE_MAX - phase)) / PHASE_MAX;
}

/* -------------------------------------------------------------------------- */
/*                              Piece Evaluations                             */
/* -------------------------------------------------------------------------- */

// Combination of material and piece square tables in one place.
static int MATERIAL_PSQT[2][NB_PIECES][64];

// Evaluates how well placed the pawns are for this color.
int evaluatePawns(Board *board, int color) {
    int score = 0;
    U64 pawns = board->pieces[PAWN] & board->colors[color];
    
    // Loop through all the pawns of this side
    while (pawns) {
        int square = poplsb(&pawns);
        
        // Material + PSQT
        score += MATERIAL_PSQT[color][PAWN][square];

        /** TODO: pawn structure goes here */
    }

    return score;
}

// Evaluates how well placed the knights are for this color.
int evaluateKnights(Board *board, int color) {
    int score = 0;
    U64 knights = board->pieces[KNIGHT] & board->colors[color];
    
    // Loop through all the knights of this side
    while (knights) {
        int square = poplsb(&knights);
        
        // Material + PSQT
        score += MATERIAL_PSQT[color][KNIGHT][square];

        // Mobility
        int count = popCount(knightAttacks(square));
        score += count * MOBILITY_VALUES[KNIGHT];
    }

    return score;
}

// Evaluates how well placed the bishops are for this color.
int evaluateBishops(Board *board, int color) {
    int score = 0;
    U64 bishops = board->pieces[BISHOP] & board->colors[color];

    /**
     * Bishop pair value if there are exactly two bishops. My logic is, if there
     * are three bishops then there is redundancy since there are two bishops on
     * the same color. This is very rare though so likely probably doesn't change
     * anything at all. It is also possible to lose one of your bishops and then
     * promote to a bishop of the same color as your remaining bishop, but that's
     * too hard to detect and probably (hopefully) too rare...
     */
    if (popCount(bishops) == 2)
        score += BISHOP_PAIR_VALUE;
    
    // Loop through all the bishops of this side
    while (bishops) {
        int square = poplsb(&bishops);
        
        // Material + PSQT
        score += MATERIAL_PSQT[color][BISHOP][square];

        // Mobility
        int count = popCount(Bmagic(square, board->colors[BOTH]));
        score += count * MOBILITY_VALUES[BISHOP];
    }

    return score;
}

// Evaluates how well placed the rooks are for this color.
int evaluateRooks(Board *board, int color) {
    int score = 0;
    U64 rooks = board->pieces[ROOK] & board->colors[color];

    // Loop through all the rooks of this side
    while (rooks) {
        int square = poplsb(&rooks);
        
        // Material + PSQT
        score += MATERIAL_PSQT[color][ROOK][square];

        // Mobility
        int count = popCount(Rmagic(square, board->colors[BOTH]));
        score += count * MOBILITY_VALUES[ROOK];
    }

    return score;
}

// Evaluates how well placed the queens are for this color.
int evaluateQueens(Board *board, int color) {
    int score = 0;
    U64 queens = board->pieces[QUEEN] & board->colors[color];
    
    // Loop through all the queens of this side
    while (queens) {
        int square = poplsb(&queens);
        
        // Material + PSQT
        score += MATERIAL_PSQT[color][QUEEN][square];

        // Mobility
        int count = popCount(Qmagic(square, board->colors[BOTH]));
        score += count * MOBILITY_VALUES[QUEEN];
    }

    return score;
}

// Evaluates how well placed the king is for this color.
int evaluateKing(Board *board, int color) {
    int score = 0;

    // There *should* only ever be one king
    assert(popCount(board->pieces[KING] & board->colors[color]) == 1);
    int kingSquare = getlsb(board->pieces[KING] & board->colors[color]);

    // PSQT
    score += MATERIAL_PSQT[color][KING][kingSquare];

    // Virtual mobility as a queen, acts as a crude king safety eval
    int count = popCount(Qmagic(kingSquare, board->colors[color]));
    score += count * MOBILITY_VALUES[KING];

    return score;
}

/* -------------------------------------------------------------------------- */
/*                              Evaluation (main)                             */
/* -------------------------------------------------------------------------- */

/**
 * Initialises the evaluation by initialising MATERIAL_PSQT, an array of the sum
 * of PSQT and material, with squares mirrored for white.
 */
void initEvaluation() {
    for (int piece = PAWN; piece <= KING; piece++) {
        for (int sq = 0; sq < 64; sq++) {
            int mat = MATERIAL_VALUES[piece];
            // White (mirrored)
            MATERIAL_PSQT[WHITE][piece][sq] = PSQT_BASE[piece][MIRROR_SQ(sq)] + mat;

            // Black
            MATERIAL_PSQT[BLACK][piece][sq] = PSQT_BASE[piece][sq] + mat;
        }
    }
}

void printScore(int score) {
    int scoreMG = ScoreMG(score);
    int scoreEG = ScoreEG(score);
    printf("MG: %d, EG: %d\n", scoreMG, scoreEG);
}

// Evaluation of the current board state, from the side to move's POV
int evaluate(Board *board) {
    // Calculate everything from the our POV (the side to move).
    // Negamax relies on this fact.
    int us = board->side;
    int them = !board->side;

    // Start evaluating!
    int score = 0;

    // Pawns
    score += evaluatePawns(board, us) - evaluatePawns(board, them);

    // Knights
    score += evaluateKnights(board, us) - evaluateKnights(board, them);

    // Bishops
    score += evaluateBishops(board, us) - evaluateBishops(board, them);

    // Rooks
    score += evaluateRooks(board, us) - evaluateRooks(board, them);

    // Queens
    score += evaluateQueens(board, us) - evaluateQueens(board, them);

    // Kings
    score += evaluateKing(board, us) - evaluateKing(board, them);

    // Add tempo bonus for side to move
    score += (board->side == WHITE) ? TEMPO : -TEMPO;

    // Taper the score between midgame and endgame
    int phase = getGamePhase(board);
    score = taper(score, phase);
    return score;
}

/* -------------------------------------------------------------------------- */
/*                             Eval debug helpers                             */
/* -------------------------------------------------------------------------- */

// Converts an evaluation from centipawns (int) to full pawns (float).
float evalToPawns(int centipawns) {
    return (float) centipawns / 100.0;
}

/**
 * Debug function to print the evaluation of the board, with a breakdown of how
 * each piece contributes. This function is really bad and I hate it.
 */
void printEvaluation(Board *board) {
    typedef struct {
        int midgame;
        int endgame;
        int tapered;
    } EvalResult;

    puts("Evaluation for board:");
    printBoard(board);
    int phase = getGamePhase(board);

    // Array of all evaluation functions
    int (*evalFunction[NB_PIECES])() = {
        evaluatePawns,
        evaluateKnights,
        evaluateBishops,
        evaluateRooks,
        evaluateQueens,
        evaluateKing,
    };
    
    // Get evaluation scores for each piece on each side
    // evaluations[piece][side];
    EvalResult evals[NB_PIECES][2];
    int whiteTotal, blackTotal = 0;
    for (int piece = PAWN; piece <= KING; piece++) {
        // Get white and black evaluations for this piece
        int whiteEval = evalFunction[piece](board, WHITE);
        int blackEval = evalFunction[piece](board, BLACK);

        // Store white evaluations (midgame, endgame, tapered)
        evals[piece][WHITE].midgame = ScoreMG(whiteEval);
        evals[piece][WHITE].endgame = ScoreEG(whiteEval);
        evals[piece][WHITE].tapered = taper(whiteEval, phase);
        
        // Store black evaluations (midgame, endgame, tapered)
        evals[piece][BLACK].midgame = ScoreMG(blackEval);
        evals[piece][BLACK].endgame = ScoreEG(blackEval);
        evals[piece][BLACK].tapered = taper(blackEval, phase);

        // Add to total
        whiteTotal += whiteEval;
        blackTotal += blackEval;
    }

    // Print a beautiful table
    const char* pieceNames[] = {"Pawns", "Knights", "Bishops", "Rooks", "Queens", "King"};
    puts("|---------------------------------------------------------------------------------------------------------------|");
    puts("|                                         Evaluation Breakdown                                                  |");
    puts("|---------------------------------------------------------------------------------------------------------------|");
    printf("| %-13s | %-29s | %-29s | %-29s | \n",
        "Piece Type", "White Evaluation", "Black Evaluation", "Imbalance");
    puts("|---------------------------------------------------------------------------------------------------------------|");
    for (int piece = PAWN; piece <= KING; piece++) {
        float whiteMG = evalToPawns(evals[piece][WHITE].midgame);
        float whiteEG = evalToPawns(evals[piece][WHITE].endgame);
        float whiteTP = evalToPawns(evals[piece][WHITE].tapered);

        float blackMG = evalToPawns(evals[piece][BLACK].midgame);
        float blackEG = evalToPawns(evals[piece][BLACK].endgame);
        float blackTP = evalToPawns(evals[piece][BLACK].tapered);

        float midgame = whiteMG - blackMG;
        float endgame = whiteEG - blackEG;
        float tapered = whiteTP - blackTP;

        printf("| %-13s | %+-6.2f    %6.2f MG %6.2f EG | %+-6.2f    %6.2f MG %6.2f EG | %+-6.2f    %6.2f MG %6.2f EG |\n",
            pieceNames[piece],
            whiteTP,
            whiteMG,
            whiteEG,
            blackTP,
            blackMG,
            blackEG,
            tapered,
            midgame,
            endgame
        );
    }
    puts("|---------------------------------------------------------------------------------------------------------------|");

    // Print and add tempo bonus
    int tempoWhite = 0;
    int tempoBlack = 0;
    if (board->side == WHITE) {
        tempoWhite = TEMPO;
    } else {
        tempoBlack = TEMPO;
    }
    int tempoNet = tempoWhite - tempoBlack;
    printf("| %-13s | %+-29.2f | %+-29.2f | %+-29.2f |\n",
        "Tempo Bonus", evalToPawns(ScoreMG(tempoWhite)), evalToPawns(ScoreMG(tempoBlack)), evalToPawns(ScoreMG(tempoNet))
    );

    puts("|---------------------------------------------------------------------------------------------------------------|");
    // Print total
    int total = whiteTotal - blackTotal + tempoNet;
    float totalMG = evalToPawns(ScoreMG(total));
    float totalEG = evalToPawns(ScoreEG(total));
    float totalTP = evalToPawns(taper(total, phase));

    printf("| %-13s | %+-6.2f    %6.2f MG %6.2f EG | %+-6.2f    %6.2f MG %6.2f EG | %+-6.2f    %6.2f MG %6.2f EG |\n",
        "Total Eval",
        evalToPawns(taper(whiteTotal, phase)),
        evalToPawns(ScoreMG(whiteTotal)),
        evalToPawns(ScoreEG(whiteTotal)),
        evalToPawns(taper(blackTotal, phase)),
        evalToPawns(ScoreMG(blackTotal)),
        evalToPawns(ScoreEG(blackTotal)),
        totalTP,
        totalMG,
        totalEG
    );
    puts("|---------------------------------------------------------------------------------------------------------------|");

    // Check if our evaluation is the same as the real one
    int finalEval = taper(total, phase);
    if (board->side == BLACK) {
        finalEval = -finalEval;
    }
    assert(finalEval == evaluate(board));
}


// Leftover function from when I converted seperate MG and EG tables into one table.
// void formatPacked() {
//     int scoreWidth = 5;
//     const char *pieceStrings[] = {
//         "PAWN",
//         "KNIGHT",
//         "BISHOP",
//         "ROOK",
//         "QUEEN",
//         "KING"
//     };

//     //  definition
//     puts("static const int PSQT[NB_PIECES][64] = {");
//     for (int piece = PAWN; piece <= KING; piece++) {
//         // Each piece
//         printf("    // %s\n", pieceStrings[piece]);
//         puts("    {");

//         for (int square = 0; square < 64; square++) {
//             // Tab on first score of each line
//             if (fileOf(square) == 0) {
//                 printf("        ");
//             }


//             // Get midgame and endgame scores
//             int scoreMG = PSQT_MG[piece][square];
//             int scoreEG = PSQT_EG[piece][square];

//             // Format into packed score
//             printf("S(%3d, %3d), ", scoreMG, scoreEG);

//             // Line break every file 8
//             if (fileOf(square) == 7) {
//                 puts("");
//             }
//         }
//         puts("    },");
//     }
//     puts("};");
// }
