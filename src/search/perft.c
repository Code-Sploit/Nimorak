#include <board/movegen.h>
#include <board/board.h>

#include <search/perft.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define PERFT_TEST_COUNT 6

typedef struct
{
    const char *fen;

    int expected_nodes[8];

    int start_depth;
    int end_depth;
} PerftTest;

PerftTest perft_tests[6] = {
    {.fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    .start_depth = 1,
    .end_depth = 5,
    .expected_nodes = {20, 400, 8902, 197281, 4865609}},

    {.fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ",
    .start_depth = 1,
    .end_depth = 4,
    .expected_nodes = {48, 2039, 97862, 4085603}},

    {.fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ",
    .start_depth = 1,
    .end_depth = 6,
    .expected_nodes = {14, 191, 2812, 43238, 674624, 11030083}},

    {.fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    .start_depth = 1,
    .end_depth = 5,
    .expected_nodes = {6, 264, 9467, 422333, 15833292}},

    {.fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    .start_depth = 1,
    .end_depth = 4,
    .expected_nodes = {44, 1486, 62379, 2103487}},

    {.fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    .start_depth = 1,
    .end_depth = 4,
    .expected_nodes = {46, 2079, 89890, 3894594}}
};

// Recursive perft: counts all leaf nodes
long long perft(Game *game, int depth)
{
    if (depth == 0)
        return 1;

    MoveList moves;

    moves.count = 0;

    movegen_generate_legal_moves(game, &moves, 0);

    long long nodes = 0;

    for (int i = 0; i < moves.count; i++)
    {
        Move move = moves.moves[i];

        board_make_move(game, move, MAKE_MOVE_LIGHT);

        nodes += perft(game, depth - 1);
        
        board_unmake_move(game, MAKE_MOVE_LIGHT);
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

    movegen_generate_legal_moves(game, &moves, 0);

    long long total_nodes = 0;

    clock_t start = clock();

    for (int i = 0; i < moves.count; i++)
    {
        Move move = moves.moves[i];

        board_make_move(game, move, MAKE_MOVE_LIGHT);

        long long nodes = perft(game, depth - 1);
        
        board_unmake_move(game, MAKE_MOVE_LIGHT);

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

    int total_nodes = 0;  // To accumulate total nodes searched
    bool stop = false;

    clock_t start_time = clock();  // Start the overall timer for all tests

    for (int index = 0; index < PERFT_TEST_COUNT; index++)
    {
        if (stop) break;

        PerftTest test = perft_tests[index];

        printf("\nPosition [%s]\n", test.fen);

        clock_t start_time_test = clock();  // Start the timer for this specific test

        for (int depth = test.start_depth; depth <= test.end_depth; depth++)
        {
            clock_t start_time_depth = clock();  // Start the timer for this specific depth

            board_load_fen(game, test.fen);  // Load FEN into the game board

            int nodes = perft(game, depth);  // Run the perft calculation for this depth
            total_nodes += nodes;  // Accumulate the nodes searched for this test

            if (depth - 1 < 8 && nodes == test.expected_nodes[depth - 1])
            {
                clock_t end_time_this_depth = clock();  // End timer for this depth

                double elapsed_time = (double)(end_time_this_depth - start_time_depth) * 1000.0 / CLOCKS_PER_SEC;
                printf("OK\t| Test: %d\tDepth: %d\tNodes: %d/%d\tTime: %.2f ms\n", 
                       index + 1, depth, nodes, test.expected_nodes[depth - 1], elapsed_time);
            }
            else
            {
                printf("FAILED\t| Test: %d\tDepth: %d\tNodes: %d/%d\n", 
                       index + 1, depth, nodes, test.expected_nodes[depth - 1]);
                stop = true;
                break;
            }
        }

        clock_t end_time_test = clock();  // End the timer for this specific test
        double elapsed_time_test = (double)(end_time_test - start_time_test) * 1000.0 / CLOCKS_PER_SEC;
        printf("Test %d completed in %.2f ms\n", index + 1, elapsed_time_test);
    }

    clock_t end_time = clock();  // End the overall timer

    double elapsed_time_total = (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC;
    printf("\nTotal nodes searched: %d\n", total_nodes);
    printf("Total time for all tests: %.2f ms\n", elapsed_time_total);

    double average_nps = total_nodes / elapsed_time_total * 1000.0;

    printf("Average NPS: %.2f\n", average_nps);
}