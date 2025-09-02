#pragma once

#include <core/movegen.hpp>

namespace Nimorak {
    class Game;
}

namespace Perft {

    // Performs a perft test at the given depth
    long long perft(Nimorak::Game& game, int depth);

    // Runs standard perft test cases
    void runTests(Nimorak::Game& game);

    // Performs perft from the root position
    void root(Nimorak::Game& game, int depth);

} // namespace Perft