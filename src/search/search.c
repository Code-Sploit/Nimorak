#include <table/transposition.h>
#include <table/repetition.h>

#include <board/movegen.h>
#include <board/board.h>

#include <search/search.h>
#include <search/eval.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

Move search_killer_moves[SEARCH_MAX_DEPTH][2];

typedef struct
{
    Move move;

    int score;
} MoveScore;

const int search_mvv_lva_scores[5][5] = {
    // Attacker:  PAWN  KNIGHT  BISHOP  ROOK   QUEEN
    /* Victim: PAWN   */ { 900,   700,    680,   500,   100 },
    /* Victim: KNIGHT */ {2700,  2400,   2380,  2200,  1800 },
    /* Victim: BISHOP */ {2900,  2600,   2580,  2400,  2000 },
    /* Victim: ROOK   */ {4900,  4600,   4580,  4400,  4000 },
    /* Victim: QUEEN  */ {8900,  8600,   8580,  8400,  8000 }
};


static inline int search_get_mvv_lva_score(Game *game, Move move)
{
    int from = GET_FROM(move);
    int to   = GET_TO(move);

    Piece from_piece = board_get_square(game, from);
    Piece to_piece   = board_get_square(game, to);

    return search_mvv_lva_scores[GET_TYPE(to_piece) - 1][GET_TYPE(from_piece) - 1];
}

static inline int search_compare_moves(const void *a, const void *b)
{
    const MoveScore *ma = (const MoveScore *) a;
    const MoveScore *mb = (const MoveScore *) b;

    return mb->score - ma->score;
}

void search_order_moves(Game *game, MoveList *movelist, int ply)
{
    if (!game || movelist->count == 0)
        return;

    MoveScore scored_moves[256];
    int count = 0;

    for (int i = 0; i < movelist->count; i++)
    {
        Move move = movelist->moves[i];
        int score = 0;

        if (IS_CAPTURE(move))
        {
            score = search_get_mvv_lva_score(game, move);
        }
        else
        {
            // Check if move is a killer move
            if (move == search_killer_moves[ply][0])
                score = 900000;
            else if (move == search_killer_moves[ply][1])
                score = 800000;
            else
                score = 0; // Could use history heuristic later
        }

        scored_moves[count++] = (MoveScore){ move, score };
        if (count >= 256) break;
    }

    qsort(scored_moves, count, sizeof(MoveScore), search_compare_moves);

    movelist->count = count;
    for (int i = 0; i < count; i++)
        movelist->moves[i] = scored_moves[i].move;
}

int search_quiescense(Game *game, int alpha, int beta, int depth, int ply)
{
    if (!game) return 0;

    if (depth >= SEARCH_QUIESCENSE_DEPTH_LIMIT) return eval_position(game);

    int stand_pat = eval_position(game);

    if (stand_pat >= beta)
        return beta;
    if (stand_pat > alpha)
        alpha = stand_pat;

    MoveList captures;
    captures.count = 0;

    movegen_generate_legal_moves(game, &captures, 1);  // Only captures

    for (int i = 0; i < captures.count; i++)
    {
        board_make_move(game, captures.moves[i]);

        int score = -search_quiescense(game, -beta, -alpha, depth + 1, ply + 1);

        board_unmake_move(game, captures.moves[i]);

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

int search_negamax(Game *game, int initial_depth, int depth, int alpha, int beta, int ply)
{
    if (!game) return 0;

    if (repetition_check_for_threefold(game, game->zobrist_key)) return 0;

    if (depth == 0)
    {
        int score = (SEARCH_ENABLE_QUIESCENSE == 1) ? search_quiescense(game, alpha, beta, 0, ply) : eval_position(game);

        // Adjust mate scores for distance pruning (if near mate)
        if (score > MATE_SCORE - SEARCH_MAX_DEPTH)
            score = score - ply;
        else if (score < -MATE_SCORE + SEARCH_MAX_DEPTH)
            score = score + ply;

        return score;
    }

    ZobristHash key = game->zobrist_key;

    int tt_score;

    if (tt_probe(game, key, depth, alpha, beta, &tt_score))
        return tt_score;

    MoveList movelist;
    movegen_generate_legal_moves(game, &movelist, 0);

    search_order_moves(game, &movelist, ply);

    if (movelist.count == 0)
    {
        if (board_is_king_in_check(game, game->turn))
        {
            return -MATE_SCORE + ply;
        }
        else
        {
            return 0;
        }
    }

    int best_eval = -INF;
    int flag = TT_ALPHA;
    Move best_move = 0;
    int alpha_original = alpha;

    for (int i = 0; i < movelist.count; i++)
    {
        board_make_move(game, movelist.moves[i]);

        int eval = -search_negamax(game, initial_depth, depth - 1, -beta, -alpha, ply + 1);

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
            flag = TT_BETA;

            if (!IS_CAPTURE(movelist.moves[i]))
            {
                // ply is already passed, so ply - 1 for killer moves at current ply
                int killer_ply = ply;

                if (search_killer_moves[killer_ply][0] != movelist.moves[i])
                {
                    search_killer_moves[killer_ply][1] = search_killer_moves[killer_ply][0];
                    search_killer_moves[killer_ply][0] = movelist.moves[i];
                }
            }

            break;
        }
    }

    if (best_eval > alpha_original && best_eval < beta)
        flag = TT_EXACT;
    else if (best_eval <= alpha_original)
        flag = TT_ALPHA;

    tt_store(game, key, depth, best_eval, flag, best_move);

    return best_eval;
}

// Updated call in search_start to match new search_negamax signature
Move search_start(Game *game, int max_depth, int think_time_ms)
{
    if (!game) return 0;

    clock_t start_time = clock();

    Move best_move_so_far = 0;
    int best_eval_so_far = -INF;

    MoveList movelist;

    for (int depth = 1; depth <= max_depth; depth++)
    {
        double elapsed_ms = (double)(clock() - start_time) * 1000.0 / CLOCKS_PER_SEC;
        if (elapsed_ms >= think_time_ms)
            break;

        Move best_this_depth = 0;
        int eval_this_depth = -INF;
        bool completed = true;

        movegen_generate_legal_moves(game, &movelist, 0);

        search_order_moves(game, &movelist, 0); // ply=0 at root

        for (int i = 0; i < movelist.count; i++)
        {
            elapsed_ms = (double)(clock() - start_time) * 1000.0 / CLOCKS_PER_SEC;
            if (elapsed_ms >= think_time_ms)
            {
                completed = false;
                break;
            }

            Move move = movelist.moves[i];
            board_make_move(game, move);

            int score = -search_negamax(game, depth, depth - 1, -INF, INF, 1);

            board_unmake_move(game, move);

            if (score > eval_this_depth)
            {
                eval_this_depth = score;
                best_this_depth = move;
            }
        }

        if (completed && best_this_depth)
        {
            best_move_so_far = best_this_depth;
            best_eval_so_far = eval_this_depth;

            if (abs(eval_this_depth) > MATE_THRESHOLD)
            {
                int mate_in = (MATE_SCORE - abs(eval_this_depth) + 1) / 2;

                if (eval_this_depth > 0) mate_in = +mate_in;
                else mate_in = -mate_in;

                printf("info depth %d score mate %d pv %s time %.0fms\n",
                       depth, mate_in, board_move_to_string(best_this_depth),
                       (double)(clock() - start_time) * 1000.0 / CLOCKS_PER_SEC);
            }
            else
            {
                printf("info depth %d score cp %d pv %s time %.0fms\n",
                       depth, eval_this_depth, board_move_to_string(best_this_depth),
                       (double)(clock() - start_time) * 1000.0 / CLOCKS_PER_SEC);
            }
        }
        else
        {
            break;
        }
    }

    if (best_move_so_far)
        return best_move_so_far;

    movegen_generate_legal_moves(game, &movelist, 0);
    return movelist.count > 0 ? movelist.moves[0] : 0;
}
