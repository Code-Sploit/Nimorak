#pragma once

#include <core/movegen.hpp>

namespace Rune {
    class Game;
}

namespace Perft {

    // Performs a perft test at the given depth
    long long perft(Rune::Game& game, int depth);

    // Runs standard perft test cases
    void runTests(Rune::Game& game);

    // Performs perft from the root position
    void root(Rune::Game& game, int depth);

} // namespace Perft