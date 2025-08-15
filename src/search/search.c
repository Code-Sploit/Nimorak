#include <table/transposition.h>
#include <table/repetition.h>

#include <board/movegen.h>
#include <board/board.h>

#include <search/search.h>
#include <search/eval.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#define MAX_MOVES 256
#define SEARCH_THINK_TIME_MARGIN 10

Move search_killer_moves[SEARCH_MAX_DEPTH][2];

// Adjust this as needed or implement actual history heuristic later
static int search_history_heuristic[64][64] = {{0}};

// MVV-LVA scores: [victim][attacker], indexed 0..4 (pawn..queen)
const int search_mvv_lva_scores[5][5] = {
    /* Attacker: PAWN KNIGHT BISHOP ROOK QUEEN */
    /* Victim: PAWN   */ { 900,   700,    680,   500,   100 },
    /* Victim: KNIGHT */ {2700,  2400,   2380,  2200,  1800 },
    /* Victim: BISHOP */ {2900,  2600,   2580,  2400,  2000 },
    /* Victim: ROOK   */ {4900,  4600,   4580,  4400,  4000 },
    /* Victim: QUEEN  */ {8900,  8600,   8580,  8400,  8000 }
};

typedef struct
{
    Move move;
    int score;
} MoveScore;

static inline void search_check_timer(Game *game)
{
    double elapsed_ms = (double)(clock() - game->search_start_time) * 1000.0 / CLOCKS_PER_SEC;
    if (elapsed_ms >= game->search_think_time - SEARCH_THINK_TIME_MARGIN) {
        game->search_stop = 1;
    }
}

static inline double search_get_timer(Game *game)
{
    return (double)(game->search_last_depth_finished_at - game->search_last_depth_started_at) * 1000.0 / CLOCKS_PER_SEC;
}

static inline double search_get_elasped_time(Game *game)
{
    return (double)(clock() - game->search_start_time) * 1000.0 / CLOCKS_PER_SEC;
}

static inline int clamp(int val, int min, int max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static inline int search_get_mvv_lva_score(Game *game, Move move)
{
    int from = GET_FROM(move);
    int to = GET_TO(move);

    Piece from_piece = board_get_square(game, from);
    Piece to_piece = board_get_square(game, to);

    // Defensive clamp on piece types (assuming 1-based indexing: PAWN=1 .. QUEEN=5)
    int victim_type = clamp(GET_TYPE(to_piece) - 1, 0, 4);
    int attacker_type = clamp(GET_TYPE(from_piece) - 1, 0, 4);

    return search_mvv_lva_scores[victim_type][attacker_type];
}

static inline int search_compare_moves(const void *a, const void *b)
{
    const MoveScore *ma = (const MoveScore *)a;
    const MoveScore *mb = (const MoveScore *)b;
    return mb->score - ma->score; // descending order
}

void search_order_moves(Game *game, MoveList *movelist, int ply)
{
    if (!game || movelist->count == 0) return;

    MoveScore scored_moves[MAX_MOVES];

    int count = movelist->count;

    for (int i = 0; i < count; i++)
    {
        Move move = movelist->moves[i];
        int score = 0;

        if (IS_CAPTURE(move))
        {
            // MVV-LVA base score + promotion bonus
            score = search_get_mvv_lva_score(game, move);

            if (IS_PROMO(move))
                score += 10000; // boost for promotions

            if (board_move_gives_check(game, move)) 
                score += 5000; // bonus for checks
        }
        else if (move == search_killer_moves[ply][0])
        {
            score = 900000;
        }
        else if (move == search_killer_moves[ply][1])
        {
            score = 800000;
        }
        else
        {
            // More precise history heuristic based on (from, to)
            int from = GET_FROM(move);
            int to = GET_TO(move);
            score = search_history_heuristic[from][to];
        }

        scored_moves[i] = (MoveScore){move, score};
    }

    // Use insertion sort instead of qsort for small arrays
    for (int i = 1; i < count; i++)
    {
        MoveScore key = scored_moves[i];
        int j = i - 1;

        while (j >= 0 && scored_moves[j].score < key.score)
        {
            scored_moves[j + 1] = scored_moves[j];
            j--;
        }
        scored_moves[j + 1] = key;
    }

    for (int i = 0; i < count; i++)
        movelist->moves[i] = scored_moves[i].move;
}

int search_quiescense(Game *game, int alpha, int beta, int depth, int ply)
{
    if (!game) return 0;

    if (depth >= SEARCH_QUIESCENSE_DEPTH_LIMIT)
        return eval_position(game);

    int stand_pat = eval_position(game);

    if (stand_pat >= beta)
        return beta;
    if (stand_pat > alpha)
        alpha = stand_pat;

    MoveList captures = {0};
    movegen_generate_legal_moves(game, &captures, 1);  // generate only captures

    search_order_moves(game, &captures, ply);

    for (int i = 0; i < captures.count; i++)
    {
        search_check_timer(game);

        if (game->search_stop) break;

        board_make_move(game, captures.moves[i], MAKE_MOVE_LIGHT);

        int score = -search_quiescense(game, -beta, -alpha, depth + 1, ply + 1);

        board_unmake_move(game, MAKE_MOVE_LIGHT);

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

int search_negamax(Game *game, int depth, int alpha, int beta, int ply)
{
    if (!game) return 0;

    // Draw by repetition
    if (repetition_check_for_threefold(game, game->zobrist_key))
        return 0;

    if (depth == 0)
    {
        int score = (SEARCH_ENABLE_QUIESCENSE == 1) ?
            search_quiescense(game, alpha, beta, 0, ply) :
            eval_position(game);

        // Mate score adjustment for distance pruning (using a large constant threshold)
        if (score > MATE_SCORE - 1000)
            score -= ply;
        else if (score < -MATE_SCORE + 1000)
            score += ply;

        return score;
    }

    ZobristHash key = game->zobrist_key;

    int tt_score;
    if (tt_probe(game, key, depth, alpha, beta, &tt_score))
        return tt_score;

    MoveList movelist = {0};
    movegen_generate_legal_moves(game, &movelist, 0);

    search_order_moves(game, &movelist, ply);

    if (movelist.count == 0)
    {
        if (board_is_king_in_check(game, game->turn))
            return -MATE_SCORE + ply; // checkmate found
        else
            return 0; // stalemate
    }

    int best_eval = -INF;
    int flag = TT_ALPHA;
    Move best_move = 0;
    int alpha_original = alpha;

    for (int i = 0; i < movelist.count; i++)
    {
        search_check_timer(game);
        if (game->search_stop) break;
        Move move = movelist.moves[i];
        board_make_move(game, move, MAKE_MOVE_FULL);

        int eval = -search_negamax(game, depth - 1, -beta, -alpha, ply + 1);

        board_unmake_move(game, MAKE_MOVE_LIGHT);

        if (eval > best_eval)
        {
            best_eval = eval;
            best_move = move;
        }

        if (eval > alpha)
            alpha = eval;

        if (alpha >= beta)
        {
            flag = TT_BETA;

            // Update killer moves only for quiet moves (non-captures)
            if (!IS_CAPTURE(move))
            {
                if (search_killer_moves[ply][0] != move)
                {
                    search_killer_moves[ply][1] = search_killer_moves[ply][0];
                    search_killer_moves[ply][0] = move;
                }
            }

            break; // Beta cutoff
        }
    }

    if (best_eval > alpha_original && best_eval < beta)
        flag = TT_EXACT;
    else if (best_eval <= alpha_original)
        flag = TT_ALPHA;

    tt_store(game, key, depth, best_eval, flag, best_move);

    return best_eval;
}

// Iterative deepening with time control (ms)
Move search_start(Game *game, int max_depth, int think_time_ms)
{
    if (!game) return 0;

    game->search_start_time = clock();
    game->search_think_time = think_time_ms;
    game->search_stop = 0;

    Move best_move_so_far = 0;
    MoveList movelist;

    for (int depth = 1; depth <= max_depth; depth++)
    {
        search_check_timer(game);
        
        if (game->search_stop) break;

        Move best_this_depth = 0;
        int eval_this_depth = -INF;
        bool completed = true;

        game->search_last_depth_started_at = clock();

        movegen_generate_legal_moves(game, &movelist, 0);
        search_order_moves(game, &movelist, 0); // root ply = 0

        for (int i = 0; i < movelist.count; i++)
        {
            search_check_timer(game);

            if (game->search_stop)
            {
                completed = false;
                break;
            }

            Move move = movelist.moves[i];
            board_make_move(game, move, MAKE_MOVE_FULL);

            // Call negamax with ply=0 at root
            int score = -search_negamax(game, depth - 1, -INF, INF, 0);

            board_unmake_move(game, MAKE_MOVE_FULL);

            if (score > eval_this_depth)
            {
                eval_this_depth = score;
                best_this_depth = move;
            }
        }

        game->search_last_depth_finished_at = clock();

        if (completed)
        {
            if (abs(eval_this_depth) > MATE_THRESHOLD)
            {
                int mate_in = (MATE_SCORE - abs(eval_this_depth) + 1) / 2;
                if (eval_this_depth < 0) mate_in = -mate_in;

                printf("info depth %d score mate %d time %.0f ms pv %s\n",
                    depth, mate_in,
                    search_get_timer(game),
                    board_move_to_string(best_this_depth));
            }
            else
            {
                printf("info depth %d score cp %d time %.0f ms pv %s\n",
                    depth, eval_this_depth,
                    search_get_timer(game),
                    board_move_to_string(best_this_depth));
            }
        }
        else
        {
            break;
        }

        best_move_so_far = best_this_depth;
    }

    printf("info search time %.0f ms\n", search_get_elasped_time(game));

    return best_move_so_far;
}