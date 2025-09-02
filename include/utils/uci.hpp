#pragma once

#include <iostream>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>

#include <core/board.hpp>

namespace Nimorak {
    class Game;
}

namespace UCI {
    void debug(const char* file, const char* format, ...);
    void printSearchResult(int depth, int score, int timeMs, int bestMove, bool isMate);
    void uciLoop(Nimorak::Game& game);
}