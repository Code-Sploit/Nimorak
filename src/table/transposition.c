#include <table/transposition.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

bool tt_probe(Game *game, ZobristHash key, int depth, int alpha, int beta, int *out_score)
{
    if (!game) return false;

    TTEntry *entry = &game->transposition_table[key & (TT_SIZE - 1)];

    if (entry->key == key && entry->depth >= depth)
    {
        if (entry->flag == TT_EXACT)
        {
            *out_score = entry->eval;

            return true;
        }
        else if (entry->flag == TT_ALPHA && entry->eval <= alpha)
        {
            *out_score = alpha;

            return true;
        }
        else if (entry->flag == TT_BETA && entry->eval >= beta)
        {
            *out_score = beta;

            return true;
        }
    }

    return false;
}

void tt_store(Game *game, ZobristHash key, int depth, int eval, int flag, Move best_move)
{
    if (!game) return;

    TTEntry *entry = &game->transposition_table[key & (TT_SIZE - 1)];

    if (entry->depth <= depth || entry->key != key)
    {
        entry->key = key;
        entry->depth = depth;
        entry->eval = eval;
        entry->flag = flag;
        entry->best_move = best_move;
    }
}

void tt_clear(Game *game)
{
    if (game)
    {
        memset(game->transposition_table, 0, sizeof(TTEntry) * TT_SIZE);
    }
}