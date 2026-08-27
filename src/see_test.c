#include <stdio.h>

#include "board.h"
#include "move.h"
#include "see.h"
#include "see_test.h"
#include "utils.h"

typedef struct {
    const char *name;
    const char *fen;
    Move move;
    int threshold;
    bool expected;
} SeeTest;

bool seeTestSuite(void) {
    const SeeTest tests[] = {
        {
            "undefended pawn takes knight",
            "4k3/8/8/3n4/2P5/8/8/4K3 w - - 0 1",
            ConstructMove(C4, D5, CAPTURE_FLAG), 300, true
        },
        {
            "capture misses threshold by one",
            "4k3/8/8/3n4/2P5/8/8/4K3 w - - 0 1",
            ConstructMove(C4, D5, CAPTURE_FLAG), 301, false
        },
        {
            "queen loses to pawn recapture",
            "4k3/8/4p3/3p4/2Q5/8/8/4K3 w - - 0 1",
            ConstructMove(C4, D5, CAPTURE_FLAG), 0, false
        },
        {
            "equal pawn exchange reaches zero",
            "4k3/8/4p3/3p4/2P5/8/8/4K3 w - - 0 1",
            ConstructMove(C4, D5, CAPTURE_FLAG), 0, true
        },
        {
            "equal pawn exchange misses one",
            "4k3/8/4p3/3p4/2P5/8/8/4K3 w - - 0 1",
            ConstructMove(C4, D5, CAPTURE_FLAG), 1, false
        },
        {
            "promotion uses promoted piece as victim",
            "r6k/4P3/8/8/8/8/8/4K3 w - - 0 1",
            ConstructMove(E7, E8, QUEEN_PROMO_FLAG), 0, false
        },
        {
            "promotion gain reaches negative threshold",
            "r6k/4P3/8/8/8/8/8/4K3 w - - 0 1",
            ConstructMove(E7, E8, QUEEN_PROMO_FLAG), -100, true
        },
        {
            "capture promotion counts capture and promotion",
            "4k2r/6P1/8/8/8/8/8/4K3 w - - 0 1",
            ConstructMove(G7, H8, QUEEN_PROMO_FLAG | CAPTURE_FLAG), 1300, true
        },
        {
            "capture promotion misses threshold by one",
            "4k2r/6P1/8/8/8/8/8/4K3 w - - 0 1",
            ConstructMove(G7, H8, QUEEN_PROMO_FLAG | CAPTURE_FLAG), 1301, false
        },
        {
            "en passant clears captured pawn from x-ray",
            "k7/8/8/5pPK/8/8/8/5r2 w - f6 0 1",
            ConstructMove(G5, F6, EP_FLAG), 1, false
        },
        {
            "king cannot make an attacked recapture",
            "8/8/4k3/3p4/2P5/8/8/3RK3 w - - 0 1",
            ConstructMove(C4, D5, CAPTURE_FLAG), 1, true
        },
        {
            "quiet move can fail a negative threshold",
            "4k3/4p3/8/8/2N5/8/8/4K3 w - - 0 1",
            ConstructMove(C4, D6, QUIET_FLAG), -299, false
        },
    };

    Board board;
    int passed = 0;
    const int count = sizeof(tests) / sizeof(tests[0]);

    printf("========== SEE Test Suite (%d cases) ==========\n", count);
    for (int i = 0; i < count; i++) {
        parseFen(&board, (char *)tests[i].fen);
        const bool actual = see(&board, tests[i].move, tests[i].threshold);

        printf("Test %d (%s) - ", i + 1, tests[i].name);
        if (actual == tests[i].expected) {
            printf_success("PASS\n");
            passed++;
        } else {
            printf_fail("FAILED\n");
            printf("FEN: %s\n", tests[i].fen);
            printf("Move: %s, threshold: %d, expected: %d, actual: %d\n",
                moveToString(tests[i].move), tests[i].threshold,
                tests[i].expected, actual);
        }
    }

    printf("Tests passed: %d / %d\n", passed, count);
    return passed == count;
}
