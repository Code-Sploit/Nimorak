#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "perft.h"
#include "movegen.h"
#include "search.h"
#include "helper.h"

void perft_debug(GameState *game, int depth, const char *root_move_str) {
    movegen_generate_legal_moves(game);

    for (int i = 0; i < game->move_count; i++) {
        Move m = game->movelist[i];

        // Convert move to UCI string
        char *from_str = translate_square_to_string(m.from);
        char *to_str = translate_square_to_string(m.to);
        
        char move_str[6];
        
        snprintf(move_str, sizeof(move_str), "%s%s", from_str, to_str);

        if (strcmp(move_str, root_move_str) != 0) continue;

        GameState new_state;
        
        game_clone(&new_state, game);
        board_make_move(&new_state, m);
        movegen_generate_legal_moves(&new_state);
        
        long long total = 0;

        printf("Divide for move %s at depth %d:\n", move_str, depth);

        for (int j = 0; j < new_state.move_count; j++) {
            Move reply = new_state.movelist[j];

            GameState reply_state;
            
            game_clone(&reply_state, &new_state);
            board_make_move(&reply_state, reply);

            long long nodes = perft(&reply_state, depth - 2);

            char *r_from = translate_square_to_string(reply.from);
            char *r_to = translate_square_to_string(reply.to);
            
            printf("    %s%s: %lld\n", r_from, r_to, nodes);

            total += nodes;
        }

        printf("Total after %s: %lld nodes\n", move_str, total);
        
        return; // Only expand one root move
    }

    printf("Move %s not found in root position.\n", root_move_str);
}

// Recursive perft function
long long perft(GameState *game, int depth) {
    if (depth == 0) return 1;

    movegen_generate_legal_moves(game);
    
    long long nodes = 0;

    for (int i = 0; i < game->move_count; i++) {
        Move m = game->movelist[i];

        GameState new_state;
        
        game_clone(&new_state, game);  // clone current state
        board_make_move(&new_state, m);
        
        nodes += perft(&new_state, depth - 1);
    }

    return nodes;
}

// Divide function like Stockfish output
uint64_t perft_divide(GameState *game, int depth, int debug) {
    movegen_generate_legal_moves(game);
    
    long long total_nodes = 0;

    if (debug) printf("info depth %d\n", depth);

    for (int i = 0; i < game->move_count; i++) {
        Move m = game->movelist[i];

        GameState new_state;
        
        game_clone(&new_state, game);
        board_make_move(&new_state, m);
        
        long long child_nodes = perft(&new_state, depth - 1);

        char *msf = translate_square_to_string(m.from);
        char *mst = translate_square_to_string(m.to);
        
        printf("%s%s: %lld\n", msf, mst, child_nodes);

        total_nodes += child_nodes;
    }

    if (debug) printf("Nodes searched: %lld\n", total_nodes);

    return total_nodes;
}

PerftResult perft_detailed(GameState *game, int depth) {
    PerftResult result = {0};

    if (depth == 0) {
        result.nodes = 1;
        return result;
    }

    movegen_generate_legal_moves(game);

    for (int i = 0; i < game->move_count; i++) {
        Move m = game->movelist[i];

        GameState new_state;
        
        game_clone(&new_state, game);
        board_make_move(&new_state, m);

        // Classify move
        if (m.capture) result.captures++;
        if (m.promotion_piece) result.promotions++;
        if (m.is_castle_king_side || m.is_castle_queen_side) result.castles++;
        if (m.is_en_passant) result.en_passants++;

        // Detect if move gives check
        movegen_generate_legal_moves(&new_state);
        
        if (is_king_in_check(&new_state, !game->turn)) {
            result.checks++;
        }

        // Quiet move if not any of the above
        if (!m.capture && !m.promotion_piece && !m.is_castle_king_side && !m.is_castle_queen_side && !m.is_en_passant) {
            result.quiet_moves++;
        }

        // Recurse
        PerftResult child = perft_detailed(&new_state, depth - 1);
        
        result.nodes       += child.nodes;
        result.captures    += child.captures;
        result.promotions  += child.promotions;
        result.castles     += child.castles;
        result.en_passants += child.en_passants;
        result.checks      += child.checks;
        result.quiet_moves += child.quiet_moves;
    }

    return result;
}

void perft_print_breakdown(GameState *game, int depth) {
    PerftResult r = perft_detailed(game, depth);
    printf("Perft breakdown at depth %d:\n", depth);
    printf("  Nodes:       %lld\n", r.nodes);
    printf("  Captures:    %lld\n", r.captures);
    printf("  Promotions:  %lld\n", r.promotions);
    printf("  Castles:     %lld\n", r.castles);
    printf("  En Passants: %lld\n", r.en_passants);
    printf("  Checks:      %lld\n", r.checks);
    printf("  Quiet moves: %lld\n", r.quiet_moves);
}

void perft_run_test_positions(GameState *game) {
    const PerftTest tests[] = {
        {
            .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            .expectations = {
                {1, 20},
                {2, 400},
                {3, 8902},
                {4, 197281},
                {5, 4865609}
            },
            .num_expectations = 5
        },
        {
            .fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ",
            .expectations = {
                {1, 14},
                {2, 191},
                {3, 2812},
                {4, 43238},
                {5, 674624}

            },
            .num_expectations = 5
        },
        {
            .fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ",
            .expectations = {
                {1, 44},
                {2, 1486},
                {3, 62379},
                {4, 2103487},
                {5, 89941194}
            },
            .num_expectations = 5
        },
        {
            .fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ",
            .expectations = {
                {1, 46},
                {2, 2079},
                {3, 89890},
                {4, 3894594},
                {5, 164075551}
            },
            .num_expectations = 4
        }
    };

    const int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    int total = 0;
    int passed = 0;

    for (int i = 0; i < num_tests; i++) {
        const PerftTest *test = &tests[i];

        printf("Testing position %d:\nFEN: %s\n", i + 1, test->fen);
        
        for (int j = 0; j < test->num_expectations; j++) {
            const DepthExpectation *exp = &test->expectations[j];
            
            board_load_fen(game, test->fen);
            
            uint64_t result = perft_divide(game, exp->depth, false);
            
            bool ok = (result == exp->expected);
            
            printf("  Depth %d: expected %llu, got %llu ... %s\n",
                   exp->depth, exp->expected, result, ok ? "PASSED" : "FAILED");
            
                   total++;
            
            if (ok) passed++;
        }
        printf("\n");
    }

    printf("Summary: %d/%d tests passed.\n", passed, total);
}