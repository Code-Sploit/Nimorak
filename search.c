#include "transposition.h"
#include "movegen.h"
#include "search.h"
#include "board.h"
#include "eval.h"

#include <stdlib.h>
#include <stdio.h>

#define SEARCH_QUIESCENSE_DEPTH_LIMIT 7
#define SEARCH_MAX_DEPTH 64

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

    search_order_moves(game, &movelist, SEARCH_INITIAL_DEPTH - depth);

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

            if (!IS_CAPTURE(movelist.moves[i]))
            {
                int ply = SEARCH_INITIAL_DEPTH - depth;

                if (search_killer_moves[ply][0] != movelist.moves[i])
                {
                    search_killer_moves[ply][1] = search_killer_moves[ply][0];
                    search_killer_moves[ply][0] = movelist.moves[i];
                }
            }

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

    search_order_moves(game, &movelist, SEARCH_INITIAL_DEPTH - initial_depth);

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