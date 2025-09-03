#include <core/eval.hpp>
#include <core/board.hpp>
#include <core/nimorak.hpp>

#include <tables/magic.hpp>

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <iostream>

namespace Nimorak {
    class Game;
}

namespace Evaluation {
    Worker::GamePhase Worker::getGamePhase(Nimorak::Game& game)
    {
        int scores[3] = {0, 0, 0}; // [OPENING, MIDDLEGAME, ENDGAME]

        // --- Strong opening indicators ---
        if (Board::hasFullMaterial(game, WHITE) && Board::hasFullMaterial(game, BLACK))
            scores[OPENING] += 5; // classic starting position, pure opening

        if (Board::pawnChainsLocked(game))
            scores[OPENING] += 3; // closed pawn structure, slow transition

        // --- Middlegame indicators ---
        if (Board::countPieces(game, QUEEN) >= 1 && Board::totalMaterial(game) < 40)
            scores[MIDDLEGAME] += 4; // reduced but queens still in play → middlegame tension

        if (Board::countPieces(game, ROOK) + Board::countPieces(game, QUEEN) >= 2)
            scores[MIDDLEGAME] += 2; // heavy pieces present → not yet endgame

        // --- Endgame indicators ---
        if (!Board::hasNonPawnMaterial(game, WHITE) && !Board::hasNonPawnMaterial(game, BLACK))
            scores[ENDGAME] += 5; // pawn-only → clear endgame

        if (!Board::hasPiece(game, QUEEN, WHITE) && !Board::hasPiece(game, QUEEN, BLACK))
            scores[ENDGAME] += 3; // queenless → pushes toward endgame

        int whiteKingRank = Helpers::rank_of(Board::findKing(game, WHITE));
        int blackKingRank = Helpers::rank_of(Board::findKing(game, BLACK));

        if (whiteKingRank > 4 || blackKingRank < 3)
            scores[ENDGAME] += 2; // active kings = endgame behavior

        // --- Decide phase ---
        int index = Helpers::findLargestOfThree(scores[OPENING], scores[MIDDLEGAME], scores[ENDGAME]);
        return static_cast<GamePhase>(index);
    }

    int Worker::getPSTFor(PieceType type, int square, GamePhase phase)
    {
        const PieceSquareTable& table = pieceSquareTables[type - 1];

        switch (phase)
        {
            case OPENING:     return table.openingValue[square];
            case MIDDLEGAME:  return table.middlegameValue[square];
            case ENDGAME:     return table.endgameValue[square];
            default:          return 0;
        }
    }

    void Worker::moduleMaterial(Nimorak::Game& game)
    {
        int moduleEval = 0;

        for (int color = WHITE; color <= BLACK; color++)
        {
            int perspective = (color == WHITE) ? 1 : -1;

            moduleEval += perspective *  pieceValues[PAWN]   * __builtin_popcountll(game.board[color][PAWN]);
            moduleEval += perspective *  pieceValues[KNIGHT] * __builtin_popcountll(game.board[color][KNIGHT]);
            moduleEval += perspective *  pieceValues[BISHOP] * __builtin_popcountll(game.board[color][BISHOP]);
            moduleEval += perspective *  pieceValues[ROOK]   * __builtin_popcountll(game.board[color][ROOK]);
            moduleEval += perspective *  pieceValues[QUEEN]  * __builtin_popcountll(game.board[color][QUEEN]);
        }

        this->eval += moduleEval;
    }

    int Worker::getMobilityScoreFor(Nimorak::Game& game, PieceType type, int square)
    {
        int score = 0;

        int phasePerspective = (getGamePhase(game) == ENDGAME) ? -1 : 1;

        switch (type)
        {
            case PAWN:   score += 5; break;
            case KNIGHT: score += __builtin_popcountll(game.attackWorker.preComputed.getKnightAttacks(square)); break;
            case BISHOP: score += __builtin_popcountll(Magic::getBishopAttacks(square, game.occupancy[BOTH])); break;
            case ROOK:   score += __builtin_popcountll(Magic::getRookAttacks(square, game.occupancy[BOTH])); break;
            case QUEEN:  score += __builtin_popcountll(Magic::getQueenAttacks(square, game.occupancy[BOTH])); break;
            case KING:   score += __builtin_popcountll(game.attackWorker.preComputed.getKingAttacks(square)) * phasePerspective; break;
            default: break;
        }

        return score;
    }
    
    void Worker::modulePST(Nimorak::Game& game)
    {
        int moduleEval = 0;

        // Compute game phase once for the whole position
        GamePhase phase = getGamePhase(game);

        Bitboard occupancy = game.occupancy[BOTH];

        while (occupancy)
        {
            int square = Helpers::pop_lsb(occupancy); // removes LSB

            Piece piece = game.boardGhost[square];

            int type  = Helpers::get_type(piece);
            int color = Helpers::get_color(piece);
            int perspective = (color == WHITE) ? 1 : -1;

            // Use mirrored square for black pieces
            int pstSquare = (color == WHITE) ? square : mirror[square];

            moduleEval += getPSTFor(type, pstSquare, phase) * perspective;
        }

        this->eval += moduleEval;
    }

    void Worker::moduleMobility(Nimorak::Game& game)
    {
        int moduleEval = 0;

        Bitboard occupancy = game.occupancy[BOTH];

        while (occupancy)
        {
            int square = Helpers::pop_lsb(occupancy);

            Piece piece = game.boardGhost[square];

            int type = Helpers::get_type(piece);
            int color = Helpers::get_color(piece);
            int perspective = (color == WHITE) ? 1 : -1;

            moduleEval += getMobilityScoreFor(game, type, square) * perspective;
        }

        this->eval += moduleEval;
    }

    // --- Eval entry point ---
    int Worker::evaluate(Nimorak::Game& game)
    {
        this->eval = 0;

        if (game.config.eval.doMaterial) moduleMaterial(game);
        if (game.config.eval.doPieceSquares) modulePST(game);
        if (game.config.eval.doMobility) moduleMobility(game);
        
        return (game.turn == WHITE) ? this->eval : -this->eval;
    }
}