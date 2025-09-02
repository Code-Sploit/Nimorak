#include <core/search.hpp>
#include <storage/transposition.hpp>
#include <storage/repetition.hpp>
#include <core/movegen.hpp>
#include <core/board.hpp>
#include <core/nimorak.hpp>
#include <core/eval.hpp>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <chrono>

#define INF 1000000

namespace Nimorak {
    class Game;
}

namespace Search {
    // -------------------------
    // Private helper implementations
    // -------------------------
    void Worker::checkTimer() {
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds> (
            Clock::now() - startTime
        ).count();

        if (elapsedMs >= (thinkTime - SEARCH_THINK_TIME_MARGIN))
            stop = true;
    }

    double Worker::getTimer() const {
        return std::chrono::duration_cast<std::chrono::milliseconds> (
            lastDepthFinishedAt - lastDepthStartedAt
        ).count();
    }

    double Worker::getElapsedTime() const {
        return std::chrono::duration_cast<std::chrono::milliseconds> (
            Clock::now() - startTime
        ).count();
    }

    int Worker::calculateExtension(Nimorak::Game& game) const {
        return Board::isKingInCheck(game, game.turn) ? 1 : 0;
    }

    int Worker::calculateReduction(Nimorak::Game& game, Move move, int depth, int index) const {
        if (depth >= 3 && index >= 5 && !Helpers::is_capture(move) && !Helpers::is_promo(move) && !Board::isKingInCheck(game, game.turn))
            return static_cast<int>(std::log(depth) * std::log(index) / 2.5);
        return 0;
    }

    int Worker::getMvvLvaScore(Nimorak::Game& game, Move move) const {
        int from = Helpers::get_from(move);
        int to = Helpers::get_to(move);
        Piece fromPiece = game.boardGhost[from];
        Piece toPiece = game.boardGhost[to];
        int victimType = std::clamp(Helpers::get_type(toPiece) - 1, 0, 4);
        int attackerType = std::clamp(Helpers::get_type(fromPiece) - 1, 0, 4);
        return mvvLvaScores[victimType][attackerType];
    }

    // -------------------------
    // Move ordering
    // -------------------------
    void Worker::orderMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply) {
        if (movelist.size() == 0) return;

        std::array<MoveScore, MAX_MOVES> scoredMoves{};
        int count = movelist.size();

        for (int i = 0; i < count; i++) {
            Move move = movelist[i];
            int score = 0;

            if (Helpers::is_capture(move)) {
                score = getMvvLvaScore(game, move);
                if (Helpers::is_promo(move)) score += SEARCH_ORDERING_PROMO_BONUS;
            }
            else if (move == killerMoves[ply][0] && game.config.search.doKillerMoves) {
                score = SEARCH_ORDERING_KILLER_MOVE1_SCORE;
            }
            else if (move == killerMoves[ply][1] && game.config.search.doKillerMoves) {
                score = SEARCH_ORDERING_KILLER_MOVE2_SCORE;
            }
            else if (game.config.search.doHeuristics) {
                score = historyHeuristic[Helpers::get_from(move)][Helpers::get_to(move)];
            }

            scoredMoves[i] = {move, score};
        }

        // Insertion sort
        for (int i = 1; i < count; i++) {
            MoveScore key = scoredMoves[i];
            int j = i - 1;
            while (j >= 0 && scoredMoves[j].score < key.score) {
                scoredMoves[j + 1] = scoredMoves[j];
                j--;
            }
            scoredMoves[j + 1] = key;
        }

        for (int i = 0; i < count; i++)
            movelist[i] = scoredMoves[i].move;
    }

    // -------------------------
    // Quiescence search
    // -------------------------
    int Worker::quiescense(Nimorak::Game& game, int depth, int alpha, int beta, int ply) {
        if (depth >= game.config.search.maximumQuiescenseDepth)
            return game.evalWorker.evaluate(game);

        int stand_pat = game.evalWorker.evaluate(game);
        if (stand_pat >= beta) return beta;
        if (stand_pat > alpha) alpha = stand_pat;

        Movegen::MoveList captures;
        game.movegenWorker.getLegalMoves(game, captures, true);
        orderMoves(game, captures, ply);

        for (int i = 0; i < captures.size(); i++) {
            checkTimer();
            if (stop) break;

            Board::makeMove(game, captures[i], MAKE_MOVE_LIGHT);
            int score = -quiescense(game, depth + 1, -beta, -alpha, ply + 1);
            Board::unmakeMove(game, MAKE_MOVE_LIGHT);

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }

        return alpha;
    }

    // -------------------------
    // Negamax
    // -------------------------
    int Worker::negamax(Nimorak::Game& game, int depth, int alpha, int beta, int ply) {
        if (game.repetitionTable.checkThreefold(game.zobristKey))
        {
            return 0;
        }

        if (depth == 0) {
            int score = game.config.search.doQuiescense ? quiescense(game, 0, alpha, beta, ply)
                                                        : game.evalWorker.evaluate(game);
            if (score > MATE_THRESHOLD) score -= ply;
            if (score < -MATE_THRESHOLD) score += ply;
            return score;
        }

        ZobristHash key = game.zobristKey;
        int tt_score;
        if (game.transpositionTable.probe(key, depth, alpha, beta, tt_score) && game.config.search.doTranspositions)
            return tt_score;

        Movegen::MoveList movelist;
        game.movegenWorker.getLegalMoves(game, movelist, false);
        orderMoves(game, movelist, ply);

        if (movelist.size() == 0) {
            return Board::isKingInCheck(game, game.turn) ? -MATE_SCORE + ply : 0;
        }

        int bestEval = -INF;
        int flag = TT_ALPHA;
        Move bestMove = 0;
        int alphaOriginal = alpha;

        for (int i = 0; i < movelist.size(); i++) {
            checkTimer();
            if (stop) break;

            Move move = movelist[i];
            Board::makeMove(game, move, MAKE_MOVE_FULL);

            int extension = calculateExtension(game);
            int reduction = (extension == 0) ? calculateReduction(game, move, depth, i) : 0;
            int realDepth = std::max(0, depth - 1 - reduction + extension);

            int eval = -negamax(game, realDepth, -beta, -alpha, ply + 1);
            Board::unmakeMove(game, MAKE_MOVE_FULL);

            if (eval > bestEval) {
                bestEval = eval;
                bestMove = move;
            }

            if (eval > alpha) alpha = eval;
            if (alpha >= beta) {
                flag = TT_BETA;
                if (!Helpers::is_capture(move) && game.config.search.doKillerMoves) {
                    if (killerMoves[ply][0] != move) {
                        killerMoves[ply][1] = killerMoves[ply][0];
                        killerMoves[ply][0] = move;
                    }
                }
                break;
            }
        }

        if (game.config.search.doTranspositions) {
            if (bestEval > alphaOriginal && bestEval < beta) flag = TT_EXACT;
            else if (bestEval <= alphaOriginal) flag = TT_ALPHA;
            game.transpositionTable.store(key, depth, bestEval, flag, bestMove);
        }

        return bestEval;
    }

    // -------------------------
    // Entry point
    // -------------------------
    Move Worker::searchPosition(Nimorak::Game& game, int initialDepth, int thinkTimeMs) {
        startTime = Clock::now();
        thinkTime = thinkTimeMs;
        stop = false;

        printf("info string Search start with initialDepth=%d thinkTime=%d ms\n", initialDepth, thinkTimeMs);

        Move bestMoveSoFar = 0;
        Movegen::MoveList movelist;

        for (int depth = 1; depth <= initialDepth; depth++) {
            checkTimer();
            if (stop) break;

            Move bestThisDepth = 0;
            int evalThisDepth = -INF;
            bool completed = true;

            lastDepthStartedAt = Clock::now();
            game.movegenWorker.getLegalMoves(game, movelist, false);

            if (movelist.size() == 1) return movelist[0];

            orderMoves(game, movelist, 0);

            for (int i = 0; i < movelist.size(); i++) {
                checkTimer();
                if (stop) { completed = false; break; }

                Move move = movelist[i];
                Board::makeMove(game, move, MAKE_MOVE_FULL);
                int score = -negamax(game, depth - 1, -INF, INF, 0);
                Board::unmakeMove(game, MAKE_MOVE_FULL);

                if (score > evalThisDepth) {
                    evalThisDepth = score;
                    bestThisDepth = move;
                }
            }

            lastDepthFinishedAt = Clock::now();

            // Print info
            if (completed && game.config.search.doInfo) {
                if (std::abs(evalThisDepth) > MATE_THRESHOLD) {
                    int mateIn = (MATE_SCORE - std::abs(evalThisDepth) + 1) / 2;
                    mateIn = (mateIn == 0) ? 1 : mateIn;
                    if (evalThisDepth < 0) mateIn = -mateIn;

                    printf("info depth %d score mate %d time %.0f ms pv %s\n",
                        depth, mateIn, getTimer(), Board::moveToString(bestThisDepth).c_str());
                } else {
                    printf("info depth %d score cp %d time %.0f ms pv %s\n",
                        depth, evalThisDepth, getTimer(), Board::moveToString(bestThisDepth).c_str());
                }
            } else if (!completed) break;

            bestMoveSoFar = bestThisDepth;
        }

        printf("info string Search timeUsed=%.0f ms\n", getElapsedTime());
        return bestMoveSoFar;
    }
} // namespace Search