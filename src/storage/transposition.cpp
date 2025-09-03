#include <storage/transposition.hpp>
#include <core/nimorak.hpp>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <algorithm>

namespace Transposition {
    inline int storeEval(int eval, int ply) {
        if (eval >  MATE_THRESHOLD) return eval + ply;
        if (eval < -MATE_THRESHOLD) return eval - ply;
        return eval;
    }

    // Adjust score back when probing from TT (make it node-relative again)
    inline int probeEval(int eval, int ply) {
        if (eval >  MATE_THRESHOLD) return eval - ply;
        if (eval < -MATE_THRESHOLD) return eval + ply;
        return eval;
    }

    bool Table::probe(ZobristHash key, int depth, int alpha, int beta, int ply, int& out_score, Move& bestMove) {
        TTEntry *entry = &this->table[key & (TT_SIZE - 1)];

        if (entry->key == key && entry->depth >= depth) {
            bestMove = entry->best_move;
            
            int corrected = probeEval(entry->eval, ply);

            if (entry->flag == TT_EXACT) {
                out_score = corrected;
                return true;
            } else if (entry->flag == TT_ALPHA && corrected <= alpha) {
                out_score = alpha;
                return true;
            } else if (entry->flag == TT_BETA && corrected >= beta) {
                out_score = beta;
                return true;
            }
        }

        return false;
    }

    void Table::store(ZobristHash key, int depth, int eval, int flag, Move best_move, int ply) {
        TTEntry *entry = &this->table[key & (TT_SIZE - 1)];

        if (entry->depth <= depth || entry->key != key) {
            entry->key = key;
            entry->depth = depth;
            entry->eval = storeEval(eval, ply);
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