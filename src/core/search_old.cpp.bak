#include <core/search.hpp>
#include <storage/transposition.hpp>
#include <storage/repetition.hpp>
#include <core/movegen.hpp>
#include <core/board.hpp>
#include <core/nimorak.hpp>
#include <core/eval.hpp>
#include <utils/uci.hpp>
#include <tables/magic.hpp>
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

    int Worker::getMvvLvaScore(Nimorak::Game& game, Move move)
    {
        int from = Helpers::get_from(move);
        int to = Helpers::get_to(move);

        Piece fromPiece = game.boardGhost[from];
        Piece toPiece = game.boardGhost[to];
        
        int victimType = std::clamp(Helpers::get_type(toPiece) - 1, 0, 4);
        int attackerType = std::clamp(Helpers::get_type(fromPiece) - 1, 0, 4);
        
        return mvvLvaScores[victimType][attackerType];
    }

    bool Worker::predictCheck(Nimorak::Game& game, Move move)
    {
        int from = Helpers::get_from(move);
        int to = Helpers::get_to(move);

        Piece piece = game.boardGhost[from];

        int type = Helpers::get_type(piece);

        if (type == KING) return false;

        Bitboard newAttacks = 0ULL;

        switch (type)
        {
            case BISHOP: newAttacks = Magic::getBishopAttacks(to, game.occupancy[BOTH]); break;
            case ROOK: newAttacks = Magic::getRookAttacks(to, game.occupancy[BOTH]); break;
            case QUEEN: newAttacks = Magic::getQueenAttacks(to, game.occupancy[BOTH]); break;
            case KNIGHT: newAttacks = game.attackWorker.preComputed.getKnightAttacks(to); break;
        }

        int kingSquareEnemy = Board::findKing(game, !game.turn);

        return (newAttacks & (1ULL << kingSquareEnemy)) != 0;
    }

    bool Worker::predictRecapture(Nimorak::Game& game, Move move)
    {
        int to = Helpers::get_to(move);

        Bitboard enemyAttacks = game.attackWorker.attackMapFull[!game.turn];

        return (enemyAttacks & (1ULL << to)) != 0;
    }

    // -------------------------
    // Move ordering
    // -------------------------
    void Worker::orderMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply)
    {
        if (movelist.size() == 0) return;

        std::array<MoveScore, MAX_MOVES> scoredMoves{};
        int count = movelist.size();

        for (int i = 0; i < count; i++)
        {
            Move move = movelist[i];
            int score = 0;

            if (predictCheck(game, move))
            {
                score = SEARCH_MOVE_CHECK_SCORE;
            }
            else if (Helpers::is_capture(move))
            {
                score = getMvvLvaScore(game, move);
                
                if (Helpers::is_promo(move)) score += SEARCH_MOVE_PROMO_SCORE;
                
                score += SEARCH_MOVE_CAPTURE_SCORE * ((predictRecapture(game, move)) ? -1 : 1);
            }
            else if (move == killerMoves[ply][0] && game.config.search.doKillerMoves)
            {
                score = SEARCH_MOVE_KILLER_1_SCORE;
            }
            else if (move == killerMoves[ply][1] && game.config.search.doKillerMoves)
            {
                score = SEARCH_MOVE_KILLER_2_SCORE;
            }
            else if (game.config.search.doHeuristics)
            {
                score = historyHeuristic[Helpers::get_from(move)][Helpers::get_to(move)];
            }

            scoredMoves[i] = {move, score};
        }

        // Insertion sort
        for (int i = 1; i < count; i++)
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

        for (int i = 0; i < count; i++)
            movelist[i] = scoredMoves[i].move;
    }

    void Worker::requestMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply, MoveRequestType requestType)
    {
        game.movegenWorker.getLegalMoves(game, movelist, requestType == QUIESCENSE);

        if (movelist.size() > 1) orderMoves(game, movelist, ply);
    }

    // -------------------------
    // Quiescence search
    // -------------------------
    int Worker::quiescense(Nimorak::Game& game, int depth, int alpha, int beta, int ply)
    {
        if (depth >= game.config.search.maximumQuiescenseDepth)
            return game.evalWorker.evaluate(game);

        int standPat = game.evalWorker.evaluate(game);

        if (standPat >= beta) return beta;
        if (standPat > alpha) alpha = standPat;

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

    // -------------------------
    // Negamax
    // -------------------------
    int Worker::negamax(Nimorak::Game& game, int depth, int alpha, int beta, int ply)
    {
        if (depth == 0)
        {
            int score = game.config.search.doQuiescense ? quiescense(game, 0, alpha, beta, ply)
                                                        : game.evalWorker.evaluate(game);
            if (score > MATE_THRESHOLD) score -= ply;
            if (score < -MATE_THRESHOLD) score += ply;
            return score;
        }

        ZobristHash key = game.zobristKey;

        Move ttMove = 0;
        int ttScore = 0;

        if (game.transpositionTable.probe(key, depth, alpha, beta, ply, ttScore, ttMove) && game.config.search.doTranspositions)
            return ttScore;

        Movegen::MoveList movelist;

        requestMoves(game, movelist, ply, NEGAMAX);

        if (movelist.size() == 0) return Board::isKingInCheck(game, game.turn) ? -MATE_SCORE + ply : 0;

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

            int eval = 0;

            if (game.repetitionTable.checkThreefold(game.zobristKey))
            {
                eval = DRAW_SCORE;
            }
            else
            {
                eval = -negamax(game, depth - 1, -beta, -alpha, ply + 1);
            }

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
            game.transpositionTable.store(key, depth, bestEval, flag, bestMove, ply);
        }

        return bestEval;
    }

    // -------------------------
    // Entry point
    // -------------------------
    Move Worker::searchPosition(Nimorak::Game& game, int initialDepth, int thinkTimeMs)
    {
        startTime = Clock::now();
        thinkTime = thinkTimeMs;
        searchCancelled = false;

        if (game.config.search.doInfo) UCI::debug(__FILE__, "start with initialDepth=%d thinkTime=%d ms", initialDepth, thinkTimeMs);

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

            if (movelist.size() == 1) return movelist[0];

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

            // Print info
            if (completed && game.config.search.doInfo)
            {
                bool isMate = (std::abs(evalThisDepth) > MATE_THRESHOLD);

                int score = evalThisDepth;

                if (isMate)
                {
                    int mateIn = (MATE_SCORE - std::abs(evalThisDepth) + 1) / 2;

                    mateIn = (mateIn == 0) ? 1 : mateIn;

                    if (evalThisDepth < 0) mateIn = -mateIn;

                    score = mateIn;
                }

                UCI::printSearchResult(depth, score, getTimer(), bestThisDepth, isMate);
            } else if (!completed) break;

            bestMoveSoFar = bestThisDepth;
        }

        if (game.config.search.doInfo) UCI::debug(__FILE__, "timeUsed=%.0f ms\n", getElapsedTime());
        return bestMoveSoFar;
    }
} // namespace Search