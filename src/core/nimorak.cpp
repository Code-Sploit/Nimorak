#include <core/nimorak.hpp>
#include <core/search.hpp>
#include <core/eval.hpp>
#include <tables/zobrist.hpp>
#include <cstdlib>
#include <cstdio>

namespace Nimorak {

    Game::Game()
        : turn(WHITE),
          enpassantSquare(-1),
          historyCount(0),
          isFirstLoad(1),
          castlingRights(CASTLING_ALL),
          config() // explicitly call default constructor to initialize all nested members
    {
        // Initialize bitboards
        for (int side = 0; side < 3; ++side) {
            for (int type = 0; type < 7; ++type)
                board[side][type] = 0ULL;
            occupancy[side] = 0ULL;
        }

        // Clear the ghost board
        for (int sq = 0; sq < 64; ++sq) boardGhost[sq] = EMPTY;

        // Allocate history array
        history = std::make_unique<State[]>(HISTORY_SIZE);

        // Initialize attack worker precomputed tables
        attackWorker.preComputed.preComputeAll();

        // Ensure attack maps are cleared
        for (int c = 0; c < 2; ++c) {
            attackWorker.attackMapFull[c] = 0ULL;
            
            for (int s = 0; s < 64; ++s)
                attackWorker.attackMap[c][s] = 0ULL;
        }

        // Initialize zobrist hasher
        Zobrist::init();
    }

    Game::~Game() {
        // All members RAII-managed, nothing else to delete
    }

} // namespace Nimorak