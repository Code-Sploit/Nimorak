#include "transposition.h"
#include "movegen.h"
#include "search.h"
#include "board.h"
#include "eval.h"

#include <stdlib.h>
#include <stdio.h>

int search_negamax(Game *game, int depth, int alpha, int beta)
{
    if (!game) return 0;

    if (depth == 0)
        return eval_position(game);

    ZobristHash key = game->zobrist_key;
    int tt_score;

    // Probe TT
    if (tt_probe(game, key, depth, alpha, beta, &tt_score))
        return tt_score;

    MoveList movelist;
    movegen_generate_legal_moves(game, &movelist);

    if (movelist.count == 0)
        return board_is_king_in_check(game, game->turn) ? -INF + (SEARCH_INITIAL_DEPTH - depth) : 0;

    int best_eval = -INF;
    int flag = TT_ALPHA; // Assume fail-low until proven otherwise
    Move best_move = 0;

    int alpha_original = alpha;  // Save original alpha for TT flag

    for (int i = 0; i < movelist.count; i++)
    {
        board_make_move(game, movelist.moves[i]);

        int eval = -search_negamax(game, depth - 1, -beta, -alpha);

        board_unmake_move(game, movelist.moves[i]);

        if (eval > best_eval)
        {
            best_eval = eval;
            best_move = movelist.moves[i];
        }

        if (eval > alpha)
            alpha = eval;

        if (alpha >= beta)
        {
            flag = TT_BETA; // Fail-high cutoff
            break;
        }
    }

    // Determine the TT flag for storing
    if (best_eval > alpha_original && best_eval < beta)
        flag = TT_EXACT;
    else if (best_eval <= alpha_original)
        flag = TT_ALPHA;
    // else flag = TT_BETA already set on cutoff

    tt_store(game, key, depth, best_eval, flag, best_move);

    return best_eval;
}

Move search_start(Game *game, int initial_depth)
{
    if (!game) return 0;

    Move best_move = 0;
    int best_eval = -INF;
    MoveList movelist;

    movegen_generate_legal_moves(game, &movelist);

    for (int i = 0; i < movelist.count; i++)
    {
        board_make_move(game, movelist.moves[i]);

        int eval = -search_negamax(game, initial_depth - 1, -INF, INF);

        board_unmake_move(game, movelist.moves[i]);

        if (eval > best_eval)
        {
            best_eval = eval;
            best_move = movelist.moves[i];
        }
    }

    return best_move;
}