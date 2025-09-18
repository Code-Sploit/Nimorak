#pragma once

#include <iostream>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>

#include <core/board.hpp>

namespace Rune {
    class Game;
}

namespace UCI {
    constexpr const char *__UCI_VERSION__ = "Rune V1.0.0";
    constexpr const char *__UCI_AUTHOR__ = "Samuel 't Hart";

    void debug(const char* file, const char* format, ...);
    void printSearchResult(int depth, int score, int timeMs, bool isMate, std::string pvCurrent);
    void uciLoop(Rune::Game& game);
}