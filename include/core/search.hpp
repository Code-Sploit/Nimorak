#pragma once

#include <core/movegen.hpp>
#include <tables/constants.hpp>
#include <ctime>
#include <array>
#include <cmath>
#include <chrono>
#include <vector>

namespace Rune {
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

            const int DRAW_SCORE = 0;
            
            const int NULL_MOVE_PRUNE_REDUCTION = 2;

            const int HISTORY_MAX       = 160000;
            const int HISTORY_SCALE_FAC = 8;

            using Clock = std::chrono::steady_clock;
            using TimePoint = std::chrono::time_point<Clock>;

            TimePoint startTime;
            TimePoint lastDepthStartedAt;
            TimePoint lastDepthFinishedAt;

            int thinkTime = 0;

            bool searchCancelled = false;

            std::array<std::array<std::array<int, 64>, 64>, 2> betaCutoffHistory {};

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

            int getMvvLvaScore(Rune::Game& game, Move move);

            bool predictCheck(Rune::Game& game, Move move);
            bool predictRecapture(Rune::Game& game, Move move);

            bool isNullMovePruneSafe(Rune::Game& game, Movegen::MoveList& movelist);

            void addBetaCutoff(Move move, int depth, int turn);
            void updateBetaCutoffHistory();

            int evaluateStaticExchange(Rune::Game& game, int to);
            int getLeastValuablePiece(Rune::Game& game, Bitboard options);
        public:
            void orderMoves(Rune::Game& game, Movegen::MoveList& movelist, int ply);
            void requestMoves(Rune::Game& game, Movegen::MoveList& movelist, int ply, MoveRequestType requestType);

            int quiescense(Rune::Game& game, int depth, int alpha, int beta, int ply, std::vector<Move>& pv);
            int negamax(Rune::Game& game, int depth, int alpha, int beta, int ply, std::vector<Move>& pv);

            Move searchPosition(Rune::Game& game, int initialDepth, int thinkTimeMs);
    };
} // namespace Search