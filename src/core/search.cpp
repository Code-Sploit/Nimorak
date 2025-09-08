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

    // -------------------------
    // Move ordering
    // -------------------------

    bool Worker::predictCheck(Nimorak::Game& game, Move move)
    {
        int enemyKingSquare = Board::findKing(game, !game.turn);

        Bitboard newAttacks = game.attackWorker.getNewAttacksForMove(game, move);

        if ((newAttacks & (1ULL << enemyKingSquare)) != 0) return true;

        return false;
    }

    bool Worker::predictRecapture(Nimorak::Game& game, Move move)
    {
        int to   = Helpers::get_to(move);

        Bitboard enemyAttacks = game.attackWorker.attackMapFull[!game.turn];

        if (enemyAttacks & (1ULL << to))
            // Opponent can recapture
            return true;
        
        return false;
    }

    int Worker::getMvvLvaScore(Nimorak::Game& game, Move move)
    {
        int from = Helpers::get_from(move);
        int to   = Helpers::get_to(move);

        Piece fPiece = game.boardGhost[from];
        Piece tPiece = game.boardGhost[to];

        int fType = Helpers::get_type(fPiece);
        int tType = Helpers::get_type(tPiece);

        return mvvLvaScores[fType - 1][tType - 1];
    }

    void Worker::orderMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply)
    {
        // Temporary container for scored moves
        std::vector<MoveScore> scored;
        scored.reserve(movelist.size());

        for (int i = 0; i < movelist.size(); i++)
        {
            Move m = movelist[i];
            int score = 0;

            // 1. TT move bonus
            if (m == this->ttMove) {
                score += SEARCH_MOVE_TT;
            }

            // 2. Captures, MVV-LVA
            if (Helpers::is_capture(m)) {
                score += SEARCH_MOVE_CAPTURE + getMvvLvaScore(game, m); // implement mvv_lva_value()
            }

            // 3. Check bonus
            if (predictCheck(game, m)) {
                score += SEARCH_MOVE_CHECK;
            }

            // 4. Good / bad capture heuristic
            if (Helpers::is_capture(m)) {
                if (!predictRecapture(game, m)) {
                    score += SEARCH_MOVE_CAPTURE_BIAS; // good capture
                } else {
                    score -= SEARCH_MOVE_CAPTURE_BIAS; // bad capture
                }
            }

            // 5. Promotion bonus
            if (Helpers::is_promo(m)) {
                score += SEARCH_MOVE_PROMOTION;
            }

            // 6. Killer moves (quiet moves that caused cutoff before)
            if (m == killerMoves[ply][0] || m == killerMoves[ply][1]) {
                score += SEARCH_MOVE_KILLER;
            }

            scored.push_back({m, score});
        }

        // Sort scored moves by descending score
        std::sort(scored.begin(), scored.end(), [](const MoveScore &a, const MoveScore &b) {
            return a.score > b.score;
        });

        // Rewrite movelist in new order
        for (size_t i = 0; i < scored.size(); i++) {
            movelist[i] = scored[i].move;
        }
    }

    void Worker::requestMoves(Nimorak::Game& game, Movegen::MoveList& movelist, int ply, MoveRequestType requestType)
    {
        game.movegenWorker.getLegalMoves(game, movelist, requestType == QUIESCENSE);

        orderMoves(game, movelist, ply);
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
            int score = game.evalWorker.evaluate(game);

            if (score > MATE_THRESHOLD) score -= ply;
            if (score < -MATE_THRESHOLD) score += ply;
            return score;
        }

        ZobristHash key = game.zobristKey;

        Move ttMove = 0;
        int ttScore = 0;

        if (game.transpositionTable.probe(key, depth, alpha, beta, ply, ttScore, ttMove) && game.config.search.doTranspositions)
            return ttScore;

        // store ttMove so requestMoves can reorder
        this->ttMove = ttMove;

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
            
            // get root moves, reorder with ttMove if available
            Move tmpBestMove = 0;
            int tmpScore = 0;
            if (game.config.search.doTranspositions) {
                game.transpositionTable.probe(game.zobristKey, depth, -INF, INF, 0, tmpScore, tmpBestMove);
                this->ttMove = tmpBestMove;
            }

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