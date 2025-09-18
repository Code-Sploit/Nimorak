#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <cstdint>
#include <random>

#include <core/board.hpp>
#include <core/rune.hpp>
#include <core/movegen.hpp>

#include <tables/helpers.hpp>

#include <storage/openingbook.hpp>

namespace OpeningBook {
    Move tryBookMove(Rune::Game& game)
    {
        if (game.ply > 12) return 0;

        // --- 1. Collect all next moves from matching lines ---
        std::unordered_set<std::string> nextMovesSet;

        for (const auto& opening : OpeningBookData::openingBook) {
            bool match = true;
            for (size_t i = 0; i < static_cast<size_t>(game.historyCount) && i < opening.size(); i++) {
                std::string historyUCIMove = Board::moveToString(game.history[i].move);
                if (historyUCIMove != opening[i]) {
                    match = false;
                    break;
                }
            }
            if (match && static_cast<size_t>(game.historyCount) < opening.size()) {
                nextMovesSet.insert(opening[game.historyCount]);
            }
        }

        if (nextMovesSet.empty()) return 0;

        // --- 2. Randomly pick one move ---
        std::vector<std::string> nextMoves(nextMovesSet.begin(), nextMovesSet.end());
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(0, nextMoves.size() - 1);

        std::string chosenUCI = nextMoves[dist(rng)];

        // --- 3. Convert to Move and return ---
        return Board::parseMove(game, chosenUCI);
    }
}
