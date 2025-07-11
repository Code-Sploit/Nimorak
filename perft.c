#include <stdio.h>
#include <string.h>

#include "perft.h"
#include "movegen.h"
#include "search.h"
#include "helper.h"

void perft_debug(GameState *game, int depth, const char *root_move_str) {
    generate_legal_moves(game);

    for (int i = 0; i < game->move_count; i++) {
        Move m = game->movelist[i];

        // Convert move to UCI string
        char *from_str = translate_square_to_string(m.from);
        char *to_str = translate_square_to_string(m.to);
        char move_str[6];
        snprintf(move_str, sizeof(move_str), "%s%s", from_str, to_str);

        if (strcmp(move_str, root_move_str) != 0) continue;

        GameState new_state;
        clone_game_state(&new_state, game);
        make_move(&new_state, m);

        generate_legal_moves(&new_state);
        long long total = 0;

        printf("Divide for move %s at depth %d:\n", move_str, depth);

        for (int j = 0; j < new_state.move_count; j++) {
            Move reply = new_state.movelist[j];

            GameState reply_state;
            clone_game_state(&reply_state, &new_state);
            make_move(&reply_state, reply);

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

    generate_legal_moves(game);
    long long nodes = 0;

    for (int i = 0; i < game->move_count; i++) {
        Move m = game->movelist[i];

        GameState new_state;
        
        clone_game_state(&new_state, game);  // clone current state

        make_move(&new_state, m);
        nodes += perft(&new_state, depth - 1);
    }

    return nodes;
}

// Divide function like Stockfish output
void perft_divide(GameState *game, int depth) {
    generate_legal_moves(game);
    long long total_nodes = 0;

    printf("info depth %d\n", depth);

    for (int i = 0; i < game->move_count; i++) {
        Move m = game->movelist[i];

        GameState new_state;
        
        clone_game_state(&new_state, game);
        make_move(&new_state, m);
        long long child_nodes = perft(&new_state, depth - 1);

        char *msf = translate_square_to_string(m.from);
        char *mst = translate_square_to_string(m.to);
        
        printf("%s%s: %lld\n", msf, mst, child_nodes);

        total_nodes += child_nodes;
    }

    printf("Nodes searched: %lld\n", total_nodes);
}

PerftResult perft_detailed(GameState *game, int depth) {
    PerftResult result = {0};

    if (depth == 0) {
        result.nodes = 1;
        return result;
    }

    generate_legal_moves(game);

    for (int i = 0; i < game->move_count; i++) {
        Move m = game->movelist[i];

        GameState new_state;
        clone_game_state(&new_state, game);
        make_move(&new_state, m);

        // Classify move
        if (m.capture) result.captures++;
        if (m.promotion_piece) result.promotions++;
        if (m.is_castle_king_side || m.is_castle_queen_side) result.castles++;
        if (m.is_en_passant) result.en_passants++;

        // Detect if move gives check
        generate_legal_moves(&new_state);
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