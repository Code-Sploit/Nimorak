#pragma once

#include <core/movegen.hpp>
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

    enum MoveRequestType {
        QUIESCENSE,
        NEGAMAX
    };

    class Worker {
        private:
            const int SEARCH_PV_MOVE_SCORE = 1000000;
            const int SEARCH_TT_MOVE_SCORE = 100000;
            const int SEARCH_KILLER1_MOVE_SCORE = 90000;
            const int SEARCH_KILLER2_MOVE_SCORE = 80000;
            const int SEARCH_PROMO_MOVE_SCORE = 10000;

            const int DRAW_SCORE = 0;

            using Clock = std::chrono::steady_clock;
            using TimePoint = std::chrono::time_point<Clock>;

            TimePoint startTime;
            TimePoint lastDepthStartedAt;
            TimePoint lastDepthFinishedAt;

            int thinkTime = 0;

            bool searchCancelled = false;

            std::array<std::array<Move, 2>, 64> killerMoves {};
            std::array<std::array<int, 64>, 64> historyHeuristic {};

            const int mvvLvaScores[5][5] = {
                {900, 700, 680, 500, 100},
                {2700, 2400, 2380, 2200, 1800},
                {2900, 2600, 2580, 2400, 2000},
                {4900, 4600, 4580, 4400, 4000},
                {8900, 8600, 8580, 8400, 8000}
            };

            Move ttMove = 0;
            Move pvMove = 0;

            void checkTimer();

            double getTimer();
            double getElapsedTime();

            int calculateSearchExtension(Nimorak::Game& game);
            int calculateLMRReduction(Nimorak::Game& game, Move move, int currDepth, int index);
            int calculateNMPReduction();
            int getMvvLvaScore(Nimorak::Game& game, Move move);

            bool canDoNMP(Nimorak::Game& game, int currDepth);
            bool canDoLMR(Nimorak::Game& game, Move move, int currDepth, int index);
        public:
            void orderMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply);
            void requestMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply, MoveRequestType requestType);

            int quiescense(Nimorak::Game& game, int depth, int alpha, int beta, int ply);
            int negamax(Nimorak::Game& game, int depth, int alpha, int beta, int ply);

            Move searchPosition(Nimorak::Game& game, int initialDepth, int thinkTimeMs);
    };
} // namespace Search