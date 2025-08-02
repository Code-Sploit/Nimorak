#include "perft.h"
#include "movegen.h"
#include "board.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Recursive perft: counts all leaf nodes
long long perft(Game *game, int depth)
{
    if (depth == 0)
        return 1;

    MoveList moves;

    movegen_generate_legal_moves(game, &moves);

    long long nodes = 0;

    for (int i = 0; i < moves.count; i++)
    {
        Move move = moves.moves[i];

        board_make_move(game, move);

        nodes += perft(game, depth - 1);
        
        board_unmake_move(game, move);
    }

    return nodes;
}

// Divide function: prints per-move node counts at root
void perft_root(Game *game, int depth)
{
    if (depth <= 0)
    {
        printf("Depth must be at least 1\n");
        return;
    }

    MoveList moves;

    movegen_generate_legal_moves(game, &moves);

    long long total_nodes = 0;

    clock_t start = clock();

    for (int i = 0; i < moves.count; i++)
    {
        Move move = moves.moves[i];

        board_make_move(game, move);

        long long nodes = perft(game, depth - 1);
        
        board_unmake_move(game, move);

        printf("%s: %lld\n", board_move_to_string(move), nodes);

        total_nodes += nodes;
    }

    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Total nodes: %lld\n", total_nodes);
    printf("Time: %.2f sec\n", time_spent);

    if (time_spent <= 0.000001)
    {
        printf("NPS: time too short to calculate accurately\n");
    }
    else
    {
        printf("NPS: %.0f\n", total_nodes / time_spent);
    }
}


// Automated perft test
void test_perft(Game *game, int depth, long long expected_nodes)
{
    printf("Testing perft depth %d...\n", depth);
    long long nodes = perft(game, depth);

    if (nodes == expected_nodes)
    {
        printf("OK: %lld nodes\n", nodes);
    }
    else
    {
        printf("FAILED: got %lld, expected %lld\n", nodes, expected_nodes);
    }
}

void perft_run_tests(Game *game)
{
    if (!game) return;

    struct {
        const char* fen;
        int depth;
        long long expected;
    } tests[] = {
        // Startpos FEN
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20 },
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400 },
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902 },
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281 },

        // Kiwipete
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ", 1, 48 },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 1", 2, 2039 },

        // En passant possible
        { "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPP2PPP/RNBQKBNR b KQkq 0 3", 1, 27 },
        { "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPP2PPP/RNBQKBNR b KQkq 0 3", 2, 1048 },

        // Castling rights removed partially
        { "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 1, 26 },
        { "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 2, 568 },
        
        // Perft test chess position 3
        { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ", 3, 2812 },
        { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ", 4, 43238},

        // Perft test chess position 4
        { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333}
    };

    int num_tests = sizeof(tests) / sizeof(tests[0]);
    double total_time = 0.0;

    printf("=== Running Perft Tests ===\n\n");

    for (int i = 0; i < num_tests; i++) {
        printf("Test %d/%d: ", i + 1, num_tests);

        board_load_fen(game, tests[i].fen);

        clock_t start = clock();
        long long nodes = perft(game, tests[i].depth);
        clock_t end = clock();

        double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
        total_time += time_spent;

        if (nodes == tests[i].expected) {
            printf("OK | Depth %d | Nodes: %lld | Time: %.2f sec\n",
                tests[i].depth, nodes, time_spent);
        } else {
            printf("FAILED | Depth %d | Got: %lld | Expected: %lld\n",
                tests[i].depth, nodes, tests[i].expected);
            return; // Stop early on failure
        }
    }

    printf("\nAll tests passed!\n");
    printf("Total time spent: %.2f seconds\n", total_time);
}
