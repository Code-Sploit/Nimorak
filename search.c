#include "constants.h"
#include "movegen.h"
#include "evaluation.h"
#include "search.h"
#include "helper.h"
#include <stdio.h>

int search_position(GameState *game, int depth, int alpha, int beta) {
    if (depth == 0) {
        return evaluate_position(game);
    }

    generate_legal_moves(game);

    int best_score = -1000000;
    int legal_moves = 0;

    for (int i = 0; i < game->move_count; i++) {
        GameState copy;
        clone_game_state(&copy, game);

        Move move = game->movelist[i];
        make_move(&copy, move);

        if (is_king_in_check(&copy, game->turn)) continue;  // Skip moves that leave king in check

        legal_moves++;

        int score = -search_position(&copy, depth - 1, -beta, -alpha);

        if (score > best_score) {
            best_score = score;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            break;  // Alpha-beta pruning
        }
    }

    // No legal moves: return checkmate or stalemate score
    if (legal_moves == 0) {
        if (is_king_in_check(game, game->turn)) {
            return -999000 + (5 - depth);  // Losing sooner is worse
        } else {
            return 0;  // Stalemate
        }
    }

    return best_score;
}

Move find_best_move(GameState *game, int depth) {
    generate_legal_moves(game);

    int alpha = -1000000;
    int beta = 1000000;
    int best_score = -1000000;

    Move best_move = (Move){.from = -1, .to = -1};

    for (int i = 0; i < game->move_count; i++) {
        GameState copy;
        clone_game_state(&copy, game);

        Move move = game->movelist[i];
        make_move(&copy, move);

        if (is_king_in_check(&copy, game->turn)) continue;

        int score = -search_position(&copy, depth - 1, -beta, -alpha);

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }

        if (score > alpha) {
            alpha = score;
        }
    }

    best_move.eval = best_score;

    return best_move;
}
