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

    Bitboard Worker::getForwardPawnMask(int square, int color)
    {
        if (square < 0 || square > 63) return 0ULL;

        Bitboard mask = 0ULL;
        
        int file = Helpers::file_of(square);
        int rank = Helpers::rank_of(square);

        if (color == WHITE)
        {
            // All ranks ahead
            mask = FILE_MASKS[file] & (~0ULL << ((rank + 1) * 8));
        }
        else
        {
            // Black pawns move downward
            mask = FILE_MASKS[file] & ((1ULL << (rank * 8)) - 1);
        }

        return mask;
    }

    Bitboard Worker::getIsolatedPawnMask(int square)
    {
        int file = Helpers::file_of(square);
        int file_left  = std::max(0, file - 1);
        int file_right = std::min(7, file + 1);

        Bitboard mask = FILE_MASKS[file_left] | FILE_MASKS[file_right];
        return mask;
    }

    Bitboard Worker::getPassedPawnMask(int square, int color)
    {
        return getForwardPawnMask(square - 1, color) | getForwardPawnMask(square, color) | getForwardPawnMask(square + 1, color);
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

    void Worker::modulePawnStructure(Nimorak::Game& game)
    {
        int moduleEval = 0;

        // Pawns bitboards for both colors
        Bitboard pawns[2] = {game.board[WHITE][PAWN], game.board[BLACK][PAWN]};
        Bitboard occupancy = game.occupancy[BOTH];

        for (int color = WHITE; color <= BLACK; color++)
        {
            Bitboard pawnBitboard = pawns[color];
            Bitboard enemyPawns = pawns[!color];
            int perspective = (color == WHITE) ? 1 : -1;

            while (pawnBitboard)
            {
                int square = Helpers::pop_lsb(pawnBitboard);
                int file = Helpers::file_of(square);
                int rank = Helpers::rank_of(square);

                Bitboard fileMask = FILE_MASKS[file];

                // --- Doubled pawns ---
                int pawnsOnFile = __builtin_popcountll(fileMask & pawns[color]);
                if (pawnsOnFile > 1)
                    moduleEval -= (pawnsOnFile - 1) * PAWN_DOUBLED_PENALTY * perspective;

                // --- Forward mask / blocked pawn ---
                Bitboard forwardMask = getForwardPawnMask(square, color);
                bool blocked = (forwardMask & occupancy) != 0;
                if (blocked)
                    moduleEval -= PAWN_BLOCKED_PENALTY * perspective;

                // --- Isolated pawn ---
                Bitboard isolatedMask = getIsolatedPawnMask(square);
                bool isolated = !(isolatedMask & pawns[color]);
                if (isolated)
                    moduleEval -= PAWN_ISOLATED_PENALTY * perspective;

                // --- Backward pawn ---
                bool backward = isolated && blocked; // simple heuristic: isolated + blocked = backward
                if (backward)
                    moduleEval -= PAWN_BACKWARD_PENALTY * perspective;

                // --- Passed pawn ---
                Bitboard passedMask = getPassedPawnMask(square, color);
                bool isPassed = !(passedMask & enemyPawns);
                if (isPassed)
                {
                    int rankBonus = (color == WHITE) ? rank : 7 - rank;
                    moduleEval += (PASSED_PAWN_BONUS + rankBonus * PASSED_PAWN_RANK_MULT) * perspective;
                }

                // --- Connected pawns bonus ---
                Bitboard connectedMask = 0ULL;
                if (file > 0) connectedMask |= FILE_MASKS[file - 1];
                if (file < 7) connectedMask |= FILE_MASKS[file + 1];
                bool connected = (connectedMask & pawns[color] & forwardMask) != 0;
                if (connected)
                    moduleEval += PAWN_CONNECTED_BONUS * perspective;
            }
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
        if (game.config.eval.doPawnStructure) modulePawnStructure(game);
        
        return (game.turn == WHITE) ? this->eval : -this->eval;
    }
}