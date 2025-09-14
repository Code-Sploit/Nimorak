#include <core/movegen.hpp>
#include <core/board.hpp>
#include <core/perft.hpp>
#include <core/rune.hpp>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>

namespace Perft {

    // Recursive perft: counts all leaf nodes
    long long perft(Rune::Game& game, int depth)
    {
        if (depth == 0)
            return 1;

        Movegen::MoveList moves;
        moves.clear();

        game.movegenWorker.getLegalMoves(game, moves, false);

        long long nodes = 0;

        for (int i = 0; i < moves.size(); i++)
        {
            Move move = moves[i];
            
            Board::makeMove(game, move, MAKE_MOVE_LIGHT);
            
            nodes += perft(game, depth - 1);
            
            Board::unmakeMove(game, MAKE_MOVE_LIGHT);
        }

        return nodes;
    }

    // Divide function: prints per-move node counts at root
    void root(Rune::Game& game, int depth)
    {
        if (depth <= 0)
        {
            printf("Depth must be at least 1\n");
            return;
        }

        Movegen::MoveList moves;
        
        game.movegenWorker.getLegalMoves(game, moves, false);

        long long total_nodes = 0;
        std::clock_t start = std::clock();

        for (int i = 0; i < moves.size(); i++)
        {
            Move move = moves[i];
            
            Board::makeMove(game, move, MAKE_MOVE_LIGHT);
            
            long long nodes = perft(game, depth - 1);
            
            Board::unmakeMove(game, MAKE_MOVE_LIGHT);

            printf("%s: %lld\n", Board::moveToString(move).c_str(), nodes);
            total_nodes += nodes;
        }

        std::clock_t end = std::clock();
        double time_spent = double(end - start) / CLOCKS_PER_SEC;

        printf("Total nodes: %lld\n", total_nodes);
        printf("Time: %.2f sec\n", time_spent);

        if (time_spent <= 1e-6)
            printf("NPS: time too short to calculate accurately\n");
        else
            printf("NPS: %.0f\n", total_nodes / time_spent);
    }

    // Automated perft tests
    void runTests(Rune::Game& game)
    {
        // You can expand this number if you add more tests
        constexpr int PERFT_TEST_COUNT = 8;

        struct PerftTest {
            const char* fen;
            long long expected_nodes[8]; // store up to 8 depths
            int start_depth;
            int end_depth;
        };

        PerftTest tests[PERFT_TEST_COUNT] = {
            {
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                {20,400,8902,197281,4865609}, 1, 5
            },
            {
                "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
                {48,2039,97862,4085603}, 1, 4
            },
            {
                "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
                {14,191,2812,43238,674624,11030083}, 1, 6
            },
            {
                "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
                {6,264,9467,422333,15833292}, 1, 5
            },
            {
                "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
                {44,1486,62379,2103487}, 1, 4
            },
            {
                "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
                {46,2079,89890,3894594}, 1, 4
            },
            // extra perft test (kiwipete)
            {
                "rnbq1rk1/ppp1bppp/3ppn2/8/3NP3/2N1BP2/PPP2QPP/R3KB1R w KQ - 4 8",
                {45,1165,52511,1419625}, 1, 4
            },
            // extra perft test (simple endgame)
            {
                "8/8/8/3k4/8/8/4K3/8 w - - 0 1",
                {8, 58, 368, 2759, 18692, 142426, 944323, 6820526}, 1, 8
            }
        };

        std::clock_t start_total = std::clock();
        printf("\n========== PERFT TESTS START ==========\n");

        bool allPassed = true;

        for (int idx = 0; idx < PERFT_TEST_COUNT && allPassed; ++idx)
        {
            PerftTest& test = tests[idx];
            printf("\n--- Test %d: FEN ---\n%s\n", idx+1, test.fen);

            for (int depth = test.start_depth; depth <= test.end_depth; ++depth)
            {
                Board::loadFen(game, test.fen);

                long long nodes = perft(game, depth);

                if (depth-1 < 8 && nodes == test.expected_nodes[depth-1])
                {
                    printf(" ✅ OK    | Depth: %2d | Nodes: %8lld / %8lld\n",
                            depth, nodes, test.expected_nodes[depth-1]);
                }
                else
                {
                    printf(" ❌ FAILED| Test: %d | Depth: %2d | Got: %8lld | Expected: %8lld\n",
                            idx+1, depth, nodes, test.expected_nodes[depth-1]);
                    allPassed = false;   // mark failure
                    break;               // stop checking deeper depths
                }
            }
        }

        double total_time_ms = (double)(std::clock() - start_total) * 1000.0 / CLOCKS_PER_SEC;
        printf("\n========== PERFT TESTS END ==========\n");
        printf("Total time: %.2f ms\n\n", total_time_ms);
    }

} // namespace Perft