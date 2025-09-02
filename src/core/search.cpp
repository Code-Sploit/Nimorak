#include <core/search.hpp>
#include <storage/transposition.hpp>
#include <storage/repetition.hpp>
#include <core/movegen.hpp>
#include <core/board.hpp>
#include <core/nimorak.hpp>
#include <core/eval.hpp>
#include <utils/uci.hpp>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <chrono>

#define INF 1000000

namespace Nimorak {
    class Game;
}

namespace Search {
    void Worker::checkTimer()
    {
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds> (
            Clock::now() - startTime
        ).count();

        if (elapsedMs >= (thinkTime - SEARCH_THINK_TIME_MARGIN))
            searchCancelled = true;
    }

    double Worker::getTimer()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds> (
            lastDepthFinishedAt - lastDepthStartedAt
        ).count();
    }

    double Worker::getElapsedTime()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds> (
            Clock::now() - startTime
        ).count();
    }

    int Worker::calculateSearchExtension(Nimorak::Game& game)
    {
        return Board::isKingInCheck(game, game.turn) ? 1 : 0;
    }

    int Worker::calculateLMRReduction(Nimorak::Game& game, Move move, int currDepth, int index)
    {
        if (currDepth >= 3 && index >= 5 && !Helpers::is_capture(move) && !Helpers::is_promo(move) && !Board::isKingInCheck(game, game.turn))
            return static_cast<int> (std::log(currDepth) * std::log(index) / 2.5);
        
        return 0;
    }

    int Worker::getMvvLvaScore(Nimorak::Game& game, Move move)
    {
        int from = Helpers::get_from(move);
        int to   = Helpers::get_to(move);

        Piece fPiece = game.boardGhost[from];
        Piece tPiece = game.boardGhost[to];

        int victimType   = std::clamp(Helpers::get_type(tPiece) - 1, 0, 4);
        int attackerType = std::clamp(Helpers::get_type(fPiece) - 1, 0, 4);

        return mvvLvaScores[victimType][attackerType];
    }

    void Worker::orderMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply)
    {
        if (movelist.size() == 0) return;

        std::array<MoveScore, MAX_MOVES> scoredMoves {};

        for (int i = 0; i < movelist.size(); i++)
        {
            Move move = movelist[i];

            int score = 0;

            if (move == this->pvMove)
            {
                score = SEARCH_PV_MOVE_SCORE;
            }
            else if (move == this->ttMove)
            {
                score = SEARCH_TT_MOVE_SCORE;
            }
            else if (Helpers::is_capture(move))
            {
                score = getMvvLvaScore(game, move);

                if (Helpers::is_promo(move)) score += SEARCH_PROMO_MOVE_SCORE;
            }
            else if (move == killerMoves[ply][0] && game.config.search.doKillerMoves)
            {
                score = SEARCH_KILLER1_MOVE_SCORE;
            }
            else if (move == killerMoves[ply][1] && game.config.search.doKillerMoves)
            {
                score = SEARCH_KILLER2_MOVE_SCORE;
            }
            else if (game.config.search.doHeuristics)
            {
                score = historyHeuristic[Helpers::get_from(move)][Helpers::get_to(move)];
            }

            scoredMoves[i] = {move, score};
        }

        for (int i = 1; i < movelist.size(); i++)
        {
            MoveScore key = scoredMoves[i];

            int j = i - 1;

            while (j >= 0 && scoredMoves[j].score < key.score)
            {
                scoredMoves[j + 1] = scoredMoves[j];

                j--;
            }

            scoredMoves[j + 1] = key;
        }

        for (int i = 0; i < movelist.size(); i++)
            movelist[i] = scoredMoves[i].move;
    }

    void Worker::requestMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply, MoveRequestType requestType)
    {
        game.movegenWorker.getLegalMoves(game, movelist, (requestType == QUIESCENSE) ? true : false);
        
        orderMoves(game, movelist, ply);
    }

    int Worker::quiescense(Nimorak::Game& game, int depth, int alpha, int beta, int ply)
    {
        if (depth >= game.config.search.maximumQuiescenseDepth)
            return game.evalWorker.evaluate(game);
        
        int staticEval = game.evalWorker.evaluate(game);

        if (staticEval >= beta) return beta;
        if (staticEval > alpha) alpha = staticEval;

        Movegen::MoveList movelist;

        requestMoves(game, movelist, ply, QUIESCENSE);

        for (int i = 0; i < movelist.size(); i++)
        {
            checkTimer();

            if (searchCancelled) break;

            Board::makeMove(game, movelist[i], MAKE_MOVE_LIGHT);

            int score = -quiescense(game, depth + 1, -beta, -alpha, ply + 1);

            Board::unmakeMove(game, MAKE_MOVE_LIGHT);

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }

        return alpha;
    }

    int Worker::negamax(Nimorak::Game& game, int depth, int alpha, int beta, int ply)
    {
        if (game.repetitionTable.checkThreefoldRecent(game.zobristKey)) return DRAW_SCORE;

        if (depth == 0)
        {
            int score = game.config.search.doQuiescense ? quiescense(game, 0, alpha, beta, ply) : game.evalWorker.evaluate(game);

            if (score > MATE_THRESHOLD)  score -= ply;
            if (score < -MATE_THRESHOLD) score += ply;

            return score;
        }

        ZobristHash key = game.zobristKey;

        Movegen::MoveList movelist;

        Move ttMove = 0;
        int ttScore = 0;

        if (game.transpositionTable.probe(key, depth, alpha, beta, ttScore, ttMove) && game.config.search.doTranspositions)
        {
            this->ttMove = ttMove;

            return ttScore;
        }

        requestMoves(game, movelist, ply, NEGAMAX);

        if (movelist.size() == 0) return Board::isKingInCheck(game, game.turn) ? -MATE_SCORE + ply : DRAW_SCORE;

        int alphaOriginal = alpha;
        int bestEval = -INF;
        int flag = TT_ALPHA;

        Move bestMove = 0;

        for (int i = 0; i < movelist.size(); i++)
        {
            checkTimer();

            if (searchCancelled) break;

            Move move = movelist[i];

            Board::makeMove(game, move, MAKE_MOVE_FULL);

            int extension = calculateSearchExtension(game);
            int reduction = (extension == 0) ? calculateLMRReduction(game, move, depth, i) : 0;
            int realDepth = std::max(0, depth - 1 - reduction + extension);

            int eval = -negamax(game, realDepth, -beta, -alpha, ply + 1);

            Board::unmakeMove(game, MAKE_MOVE_FULL);

            if (eval > bestEval)
            {
                bestEval = eval;
                bestMove = move;
            }

            if (eval > alpha) alpha = eval;

            if (alpha >= beta)
            {
                flag = TT_BETA;

                if (!Helpers::is_capture(move) && game.config.search.doKillerMoves)
                {
                    if (killerMoves[ply][0] != move)
                    {
                        killerMoves[ply][1] = killerMoves[ply][0];
                        killerMoves[ply][0] = move;
                    }
                }

                break;
            }
        }

        if (game.config.search.doTranspositions)
        {
            if (bestEval > alphaOriginal && bestEval < beta) flag = TT_EXACT;
            else if (bestEval <= alphaOriginal) flag = TT_ALPHA;

            game.transpositionTable.store(key, depth, bestEval, flag, bestMove);
        }

        return bestEval;
    }

    Move Worker::searchPosition(Nimorak::Game& game, int initialDepth, int thinkTimeMs)
    {
        startTime = Clock::now();
        thinkTime = thinkTimeMs;

        searchCancelled = false;

        UCI::debug(__FILE__, "started with initialDepth=%d thinkTime=%d ms", initialDepth, thinkTimeMs);

        Move bestMoveSoFar = 0;

        Movegen::MoveList movelist;

        for (int depth = 1; depth <= initialDepth; depth++)
        {
            checkTimer();

            if (searchCancelled) break;

            Move bestThisDepth = 0;

            int evalThisDepth = -INF;

            bool completed = true;

            lastDepthStartedAt = Clock::now();

            requestMoves(game, movelist, 0, NEGAMAX);

            for (int i = 0; i < movelist.size(); i++)
            {
                checkTimer();

                if (searchCancelled) { completed = false; break; }

                Move move = movelist[i];

                Board::makeMove(game, move, MAKE_MOVE_FULL);

                int score = -negamax(game, depth - 1, -INF, INF, 0);

                Board::unmakeMove(game, MAKE_MOVE_FULL);

                if (score > evalThisDepth)
                {
                    evalThisDepth = score;
                    bestThisDepth = move;
                }
            }

            lastDepthFinishedAt = Clock::now();

            if (completed)
            {
                bool isMate = std::abs(evalThisDepth) > MATE_THRESHOLD;

                int score = 0;

                if (isMate)
                {
                    int mateIn = (MATE_SCORE - std::abs(evalThisDepth) + 1) / 2;

                    mateIn = (mateIn == 0) ? 1 : mateIn;

                    if (evalThisDepth < 0) mateIn = -mateIn;

                    score = mateIn;
                }
                else
                {
                    score = evalThisDepth;
                }

                UCI::printSearchResult(depth, score, getTimer(), bestThisDepth, isMate);
            }
            else
            {
                break;
            }

            bestMoveSoFar = bestThisDepth;

            this->pvMove = bestMoveSoFar;
        }

        UCI::debug(__FILE__, "timeUsed=%.0f ms", getElapsedTime());

        return bestMoveSoFar;
    }
} // namespace Search