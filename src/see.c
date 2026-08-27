#include <assert.h>

#include "see.h"
#include "bitboards.h"
#include "magicmoves.h"

const int SEE_PIECE_VALUES[NB_PIECES + 1] = {
    100, 300, 310, 500, 900, 0,
    0 // EMPTY
};

/**
 * Static Exchange Evaluation (SEE).
 * This is a fast way of checking whether a move wins enough material. It is
 * much cheaper than searching every capture, at the cost of some inaccuracies
 * in edge cases such as pinned pieces.
 *
 * Reference: https://www.chessprogramming.org/SEE_-_The_Swap_Algorithm
 */
bool see(Board *board, Move move, int threshold) {
    // Extract move info.
    const int from = MoveFrom(move);
    const int to = MoveTo(move);
    const int movedPiece = board->squares[from];

    assert(move != NO_MOVE);
    assert(movedPiece >= PAWN && movedPiece <= KING);

    // Castling is not an exchange and has no immediate material value.
    if (IsCastling(move))
        return threshold <= 0;

    // En passant captures a pawn even though the destination square is empty.
    int capturedPiece = IsEnpass(move) ? PAWN : board->squares[to];
    assert(capturedPiece >= PAWN && capturedPiece <= EMPTY);

    // The moved piece is the next victim. After promotion it is the new piece,
    // not the pawn which arrived on the square.
    const int nextVictim = IsPromotion(move)
        ? MovePromotedPiece(move)
        : movedPiece;

    // Start with the captured material, promotion gain, and threshold.
    int balance = SEE_PIECE_VALUES[capturedPiece] - threshold;
    if (IsPromotion(move))
        balance += SEE_PIECE_VALUES[nextVictim] - SEE_PIECE_VALUES[PAWN];

    // If the best case is below the threshold, the exchange loses.
    if (balance < 0)
        return false;

    // If losing the moved piece still beats the threshold, the exchange wins.
    balance -= SEE_PIECE_VALUES[nextVictim];
    if (balance >= 0)
        return true;

    // Occupancy after making the move.
    U64 occupied = board->colors[BOTH];
    clearBit(&occupied, from);
    setBit(&occupied, to);

    // En passant removes its pawn from a different square, possibly opening an
    // x-ray attack.
    if (IsEnpass(move)) {
        const int capturedSquare = board->side == WHITE ? to - 8 : to + 8;
        clearBit(&occupied, capturedSquare);
    }

    // Find all attackers to the exchange square and sliders used for x-rays.
    U64 attackers = allAttackersToSquare(board, occupied, to) & occupied;
    const U64 diagonalSliders = board->pieces[BISHOP] | board->pieces[QUEEN];
    const U64 orthogonalSliders = board->pieces[ROOK] | board->pieces[QUEEN];
    int sideToCapture = !board->side;

    while (true) {
        // Get the attackers belonging to the side whose turn it is.
        U64 sideAttackers = attackers & board->colors[sideToCapture];
        if (sideAttackers == 0ULL)
            break;

        // Capture with the least valuable attacker.
        int attackerPiece;
        for (attackerPiece = PAWN; attackerPiece <= KING; attackerPiece++) {
            if (sideAttackers & board->pieces[attackerPiece])
                break;
        }
        assert(attackerPiece <= KING);

        clearBit(&occupied,
            getlsb(sideAttackers & board->pieces[attackerPiece]));

        // Removing the attacker may reveal new slider attacks.
        if (attackerPiece == PAWN || attackerPiece == BISHOP ||
            attackerPiece == QUEEN) {
            attackers |= Bmagic(to, occupied) & diagonalSliders;
        }
        if (attackerPiece == ROOK || attackerPiece == QUEEN) {
            attackers |= Rmagic(to, occupied) & orthogonalSliders;
        }
        attackers &= occupied;

        // Swap sides and negamax the material balance.
        sideToCapture = !sideToCapture;
        balance = -balance - 1 - SEE_PIECE_VALUES[attackerPiece];

        if (balance >= 0) {
            // A king cannot capture onto a square the other side still attacks.
            if (attackerPiece == KING &&
                (attackers & board->colors[sideToCapture])) {
                sideToCapture = !sideToCapture;
            }
            break;
        }
    }

    // The side with no profitable capture left loses the exchange.
    return board->side != sideToCapture;
}
