#include <storage/transposition.hpp>
#include <core/nimorak.hpp>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <algorithm>

namespace Transposition {
    bool Table::probe(ZobristHash key, int depth, int alpha, int beta, int& out_score, Move& bestMove)
    {
        TTEntry *entry = &this->table[key & (TT_SIZE - 1)];

        if (entry->key == key && entry->depth >= depth)
        {
            bestMove = entry->best_move;
            
            if (entry->flag == TT_EXACT)
            {
                out_score = entry->eval;

                return true;
            }
            else if (entry->flag == TT_ALPHA && entry->eval <= alpha)
            {
                out_score = alpha;

                return true;
            }
            else if (entry->flag == TT_BETA && entry->eval >= beta)
            {
                out_score = beta;

                return true;
            }
        }

        return false;
    }

    void Table::store(ZobristHash key, int depth, int eval, int flag, Move best_move)
    {
        TTEntry *entry = &this->table[key & (TT_SIZE - 1)];

        if (entry->depth <= depth || entry->key != key)
        {
            entry->key = key;
            entry->depth = depth;
            entry->eval = eval;
            entry->flag = flag;
            entry->best_move = best_move;
        }
    }

    void Table::clear()
    {
        std::fill(
            this->table,
            this->table + TT_SIZE,
            TTEntry{}  // value-initialize each element
        );
    }
}