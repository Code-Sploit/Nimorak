#pragma once

#include <core/movegen.hpp>
#include <core/eval.hpp>
#include <tables/constants.hpp>
#include <ctime>
#include <array>
#include <cmath>
#include <chrono>

namespace Nimorak {
    class Game;
}

namespace Search {
    struct MoveScore {
        Move move;
        int score;
    };

    class Worker {
        private:
            // Timer
            using Clock = std::chrono::steady_clock;
            using TimePoint = std::chrono::time_point<Clock>;

            TimePoint startTime;
            TimePoint lastDepthStartedAt;
            TimePoint lastDepthFinishedAt;

            int thinkTime = 0;  // in milliseconds
            bool stop = false;

            // Killer moves and history heuristic
            std::array<std::array<Move, 2>, 64> killerMoves{};
            std::array<std::array<int, 64>, 64> historyHeuristic{};

            // MVV-LVA table
            const int mvvLvaScores[5][5] = {
                {900, 700, 680, 500, 100},
                {2700, 2400, 2380, 2200, 1800},
                {2900, 2600, 2580, 2400, 2000},
                {4900, 4600, 4580, 4400, 4000},
                {8900, 8600, 8580, 8400, 8000}
            };

            // -------------------------------
            // Private helpers
            // -------------------------------
            void checkTimer();
            double getTimer() const;
            double getElapsedTime() const;

            int calculateExtension(Nimorak::Game& game) const;
            int calculateReduction(Nimorak::Game& game, Move move, int depth, int index) const;
            int getMvvLvaScore(Nimorak::Game& game, Move move) const;

        public:
            // -------------------------------
            // Move ordering
            // -------------------------------
            void orderMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply);

            // -------------------------------
            // Search algorithms
            // -------------------------------
            int quiescense(Nimorak::Game& game, int depth, int alpha, int beta, int ply);
            int negamax(Nimorak::Game& game, int depth, int alpha, int beta, int ply);

            // -------------------------------
            // Entry point
            // -------------------------------
            Move searchPosition(Nimorak::Game& game, int initialDepth, int thinkTimeMs);
    };
} // namespace Search