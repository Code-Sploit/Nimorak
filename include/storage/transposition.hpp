#pragma once

#include <tables/constants.hpp>
#include <stdbool.h>

#define TT_EXACT 0
#define TT_ALPHA 1
#define TT_BETA  2

namespace Transposition {

    struct TTEntry {
        ZobristHash key;    // Position key
        int depth;          // Depth at which entry was stored
        int eval;           // Evaluation
        int flag;           // TT_EXACT / TT_ALPHA / TT_BETA
        Move best_move;     // Best move found
    };

    class Table {
        private:
            const static size_t TT_SIZE = (1 << 8);
            TTEntry table[TT_SIZE];  // Fixed-size transposition table

        public:
            bool probe(ZobristHash key, int depth, int alpha, int beta, int& out_score, Move& bestMove);
            void store(ZobristHash key, int depth, int eval, int flag, Move best_move);
            void clear();
    };

} // namespace Transposition