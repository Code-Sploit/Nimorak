#include "transposition.h"
#include "movegen.h"
#include "search.h"
#include "board.h"
#include "eval.h"

#include <stdlib.h>
#include <stdio.h>

#define SEARCH_QUIESCENSE_DEPTH_LIMIT 3

void search_order_moves(Game *game, MoveList *movelist)
{
    if (!game || movelist->count == 0)
        return;

    MoveList ordered_moves;
    ordered_moves.count = 0;  // Initialize count!

    // First, add all captures
    for (int i = 0; i < movelist->count; i++)
    {
        Move move = movelist->moves[i];
        if (IS_CAPTURE(move))
        {
            ordered_moves.moves[ordered_moves.count++] = move;
            if (ordered_moves.count >= 256) break; // Safety
        }
    }

    // Then, add all non-captures (quiet moves)
    for (int i = 0; i < movelist->count && ordered_moves.count < 256; i++)
    {
        Move move = movelist->moves[i];
        if (!IS_CAPTURE(move))
        {
            ordered_moves.moves[ordered_moves.count++] = move;
        }
    }

    // Copy back
    for (int i = 0; i < ordered_moves.count; i++)
    {
        movelist->moves[i] = ordered_moves.moves[i];
    }
    movelist->count = ordered_moves.count;
}

int search_quiescense(Game *game, int alpha, int beta, int depth)
{
    if (!game) return 0;

    if (depth >= SEARCH_QUIESCENSE_DEPTH_LIMIT) return eval_position(game); // Safety limit

    int stand_pat = eval_position(game);

    if (stand_pat >= beta)
        return beta;
    if (stand_pat > alpha)
        alpha = stand_pat;

    MoveList captures;

    movegen_generate_legal_moves(game, &captures, 1);

    for (int i = 0; i < captures.count; i++)
    {
        board_make_move(game, captures.moves[i]);

        int score = -search_quiescense(game, -beta, -alpha, depth + 1);

        board_unmake_move(game, captures.moves[i]);

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

int search_negamax(Game *game, int depth, int alpha, int beta)
{
    if (!game) return 0;

    if (depth == 0)
        return search_quiescense(game, alpha, beta, 0);

    ZobristHash key = game->zobrist_key;
    int tt_score;

    // Probe TT
    if (tt_probe(game, key, depth, alpha, beta, &tt_score))
        return tt_score;

    MoveList movelist;
    movegen_generate_legal_moves(game, &movelist, 0);

    search_order_moves(game, &movelist);

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

    movegen_generate_legal_moves(game, &movelist, 0);

    search_order_moves(game, &movelist);

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