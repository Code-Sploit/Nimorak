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
    {
        return eval_position(game);
    }

    MoveList movelist;

    movegen_generate_legal_moves(game, &movelist);

    if (movelist.count == 0)
    {
        return board_is_king_in_check(game, game->turn) ? -INF + (SEARCH_INITIAL_DEPTH - depth) : 0;
    }

    int best_eval = -INF;

    for (int i = 0; i < movelist.count; i++)
    {
        board_make_move(game, movelist.moves[i]);

        int eval = -search_negamax(game, depth - 1, -beta, -alpha);

        board_unmake_move(game, movelist.moves[i]);

        if (eval > best_eval)
            best_eval = eval;
        
        if (eval > alpha)
            alpha = eval;
        
        if (alpha >= beta)
            break;
    }

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