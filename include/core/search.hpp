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
            const int SEARCH_MOVE_TT             = 1000000;
            const int SEARCH_MOVE_CAPTURE        = 900000;
            const int SEARCH_MOVE_CHECK          = 800000;
            const int SEARCH_MOVE_CAPTURE_BIAS   = 25000;
            const int SEARCH_MOVE_PROMOTION      = 80000;
            const int SEARCH_MOVE_KILLER         = 100000;

            const int DRAW_SCORE = 0;
            
            const int NULL_MOVE_PRUNE_REDUCTION = 2;

            const int HISTORY_MAX       = 16000;
            const int HISTORY_SCALE_DIV = 6;

            using Clock = std::chrono::steady_clock;
            using TimePoint = std::chrono::time_point<Clock>;

            TimePoint startTime;
            TimePoint lastDepthStartedAt;
            TimePoint lastDepthFinishedAt;

            int thinkTime = 0;

            bool searchCancelled = false;

            std::array<std::array<Move, 2>, 64> killerMoves {};
            std::array<std::array<std::array<int, 64>, 64>, 2> historyHeuristic {};

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

            int getMvvLvaScore(Nimorak::Game& game, Move move);

            bool predictCheck(Nimorak::Game& game, Move move);
            bool predictRecapture(Nimorak::Game& game, Move move);

            bool isNullMovePruneSafe(Nimorak::Game& game, Movegen::MoveList& movelist);
        public:
            void orderMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply);
            void requestMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply, MoveRequestType requestType);

            int quiescense(Nimorak::Game& game, int depth, int alpha, int beta, int ply);
            int negamax(Nimorak::Game& game, int depth, int alpha, int beta, int ply);

            Move searchPosition(Nimorak::Game& game, int initialDepth, int thinkTimeMs);
    };
} // namespace Search