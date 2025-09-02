#include <core/movegen.hpp>
#include <core/attack.hpp>
#include <core/board.hpp>

#include <tables/magic.hpp>

#include <iostream>

namespace Nimorak {
    class Game;
}

namespace Movegen {
    void Worker::addPromotionMoves(MoveList& moves, int from, int to, int isCapture)
    {
        moves.add(Helpers::move(from, to, QUEEN, isCapture, 1, 0, 0, 0, 0));
        moves.add(Helpers::move(from, to, ROOK, isCapture, 1, 0, 0, 0, 0));
        moves.add(Helpers::move(from, to, BISHOP, isCapture, 1, 0, 0, 0, 0));
        moves.add(Helpers::move(from, to, KNIGHT, isCapture, 1, 0, 0, 0, 0));
    }

    void Worker::getPawnMoves(Nimorak::Game& game, MoveList& moves, bool onlyCaptures)
    {
        int color = game.turn;

        Bitboard ownOcc = game.occupancy[color];
        Bitboard oppOcc = game.occupancy[!color];
        Bitboard allOcc = ownOcc | oppOcc;

        Bitboard pawns = game.board[color][PAWN];

        int pushDir = (color == WHITE) ? 8 : -8;
        int dblPushDir = (color == WHITE) ? 16 : -16;
        Bitboard promoteMask = (color == WHITE) ? RANK_8 : RANK_1;
        Bitboard startRankMask = (color == WHITE) ? RANK_2 : RANK_7;

        while (pawns)
        {
            int from = __builtin_ctzll(pawns);
            pawns &= pawns - 1;
            Bitboard fromBb = 1ULL << from;

            // Quiet moves
            if (!onlyCaptures)
            {
                int toSq = from + pushDir;
                if (toSq >= 0 && toSq < 64)
                {
                    Bitboard toBb = 1ULL << toSq;
                    if (!(toBb & allOcc))
                    {
                        if (toBb & promoteMask)
                            addPromotionMoves(moves, from, toSq, 0);
                        else
                            moves.add(Helpers::move(from, toSq, 0, 0, 0, 0, 0, 0, 0));

                        // Double push (check intermediate square)
                        if (fromBb & startRankMask)
                        {
                            int toSq2 = from + dblPushDir;
                            if (toSq2 >= 0 && toSq2 < 64)
                            {
                                Bitboard toBb2 = 1ULL << toSq2;
                                if (!(toBb2 & allOcc) && !(toBb & allOcc))
                                    moves.add(Helpers::move(from, toSq2, 0, 0, 0, 0, 0, 1, 0));
                            }
                        }
                    }
                }
            }

            // Captures
            Bitboard leftCaptures, rightCaptures;
            if (color == WHITE) {
                leftCaptures  = (fromBb & ~FILE_A) << 7;
                rightCaptures = (fromBb & ~FILE_H) << 9;
            } else {
                leftCaptures  = (fromBb & ~FILE_A) >> 9;
                rightCaptures = (fromBb & ~FILE_H) >> 7;
            }

            Bitboard attacks = (leftCaptures | rightCaptures) & oppOcc;
            while (attacks)
            {
                int toSq = __builtin_ctzll(attacks);
                attacks &= attacks - 1;

                if ((1ULL << toSq) & promoteMask)
                    addPromotionMoves(moves, from, toSq, 1);
                else
                    moves.add(Helpers::move(from, toSq, 0, 1, 0, 0, 0, 0, 0));
            }

            // En passant (king safety must still be checked in legal move filtering)
            if (game.enpassantSquare != -1)
            {
                Bitboard epBb = 1ULL << game.enpassantSquare;
                if ((leftCaptures & epBb) || (rightCaptures & epBb))
                    moves.add(Helpers::move(from, game.enpassantSquare, 0, 1, 0, 1, 0, 0, 0));
            }
        }
    }

    void Worker::getKnightMoves(Nimorak::Game& game, MoveList& moves, bool onlyCaptures)
    {
        int color = game.turn;

        Bitboard knights = game.board[color][KNIGHT];
        Bitboard friendly = game.occupancy[color];
        Bitboard oppOccupancy = game.occupancy[!color];

        while (knights)
        {
            int square = __builtin_ctzll(knights);
            knights &= knights - 1;

            // Precomputed attacks for the knight, remove friendly squares
            Bitboard attacks = game.attackWorker.preComputed.getKnightAttacks(square) & ~friendly;

            if (onlyCaptures)
            {
                // Keep only opponent pieces
                attacks &= oppOccupancy;
            }

            // Iterate over all attacks safely
            while (attacks)
            {
                int to = __builtin_ctzll(attacks);
                attacks &= attacks - 1;  // clear the bit to avoid infinite loop

                int isCapture = (oppOccupancy >> to) & 1ULL;
                moves.add(Helpers::move(square, to, 0, isCapture, 0, 0, 0, 0, 0));
            }
        }
    }

    void Worker::getKingMoves(Nimorak::Game& game, MoveList& moves, bool onlyCaptures)
    {
        int color = game.turn;

        Bitboard king = game.board[color][KING];
        Bitboard friendly = game.occupancy[color];
        Bitboard oppOccupancy = game.occupancy[!color];

        while (king)
        {
            int square = __builtin_ctzll(king);

            king &= king - 1;

            Bitboard attacks = game.attackWorker.preComputed.getKingAttacks(square) & ~friendly;

            if (onlyCaptures)
            {
                attacks &= oppOccupancy;

                while (attacks)
                {
                    int to = __builtin_ctzll(attacks);

                    attacks &= attacks - 1;

                    moves.add(Helpers::move(square, to, 0, 1, 0, 0, 0, 0, 0));
                }
            }
            else
            {
                while (attacks)
                {
                    int to = __builtin_ctzll(attacks);
                    int isCapture = (oppOccupancy >> to) & 1ULL;

                    attacks &= attacks - 1;

                    moves.add(Helpers::move(square, to, 0, isCapture, 0, 0, 0, 0, 0));
                }
            }
        }
    }

    void Worker::getSlidingMoves(Nimorak::Game& game, MoveList& moves, PieceType type, bool onlyCaptures)
    {
        int color = game.turn;
        
        Bitboard sliders = game.board[color][type];
        Bitboard occupancy = game.occupancy[BOTH];
        Bitboard friendly = game.occupancy[color];
        Bitboard oppOccupancy = game.occupancy[!color];

        while (sliders)
        {
            int from = __builtin_ctzll(sliders);

            sliders &= sliders - 1;

            Bitboard attacks = 0ULL;

            switch (type)
            {
                case BISHOP:
                    attacks = Magic::getBishopAttacks(from, occupancy);
                    break;
                case ROOK:
                    attacks = Magic::getRookAttacks(from, occupancy);
                    break;
                case QUEEN:
                    attacks = Magic::getQueenAttacks(from, occupancy);
                    break;
                default:
                    continue; // Not a sliding piece
            }

            // Remove own pieces
            attacks &= ~friendly;

            if (onlyCaptures) {
                // Captures only → keep only opponent pieces
                Bitboard caps = attacks & oppOccupancy;

                while (caps)
                {
                    int to = __builtin_ctzll(caps);

                    caps &= caps - 1;

                    moves.add(Helpers::move(from, to, 0, 1, 0, 0, 0, 0, 0));
                }
            } else {
                while (attacks)
                {
                    int to = __builtin_ctzll(attacks);
                    int is_capture = (oppOccupancy >> to) & 1ULL;

                    attacks &= attacks - 1;

                    moves.add(Helpers::move(from, to, 0, is_capture, 0, 0, 0, 0, 0));
                }
            }
        }
    }

    bool Worker::canCastleThroughBitboard(int square, Bitboard occupancy, Bitboard enemyAttacks)
    {
        return !((occupancy >> square) & 1ULL) && !((enemyAttacks >> square) & 1ULL);
    }

    void Worker::getCastleMoves(Nimorak::Game& game, MoveList& moves)
    {
        int color = game.turn;
        
        if (!Board::hasCastlingRights(game, color)) return;

        int kingSquare = Board::findKing(game, color);

        Bitboard occupied = game.occupancy[BOTH];
        Bitboard enemyAttacks = game.attackWorker.attackMapFull[!color];

        // If king is in check, no castling
        if ((enemyAttacks >> kingSquare) & 1ULL) return;

        // Kingside
        int kingsideRight = (color == WHITE) ? WHITE_KINGSIDE : BLACK_KINGSIDE;

        if (Board::hasCastlingRightsSide(game, kingsideRight))
        {
            int f_sq = kingSquare + 1;
            int g_sq = kingSquare + 2;

            if (canCastleThroughBitboard(f_sq, occupied, enemyAttacks) &&
                canCastleThroughBitboard(g_sq, occupied, enemyAttacks))
            {
                moves.add(Helpers::move(kingSquare, g_sq, 0, 0, 0, 0, 0, 0, 1));
            }
        }

        // Queenside
        int queenside_right = (color == WHITE) ? WHITE_QUEENSIDE : BLACK_QUEENSIDE;
        if (Board::hasCastlingRightsSide(game, queenside_right)) {
            int d_sq = kingSquare - 1;
            int c_sq = kingSquare - 2;
            int b_sq = kingSquare - 3;

            // b_sq only needs to be empty, no attack requirement
            if (canCastleThroughBitboard(d_sq, occupied, enemyAttacks) &&
                canCastleThroughBitboard(c_sq, occupied, enemyAttacks) &&
                !((occupied >> b_sq) & 1ULL))
            {
                moves.add(Helpers::move(kingSquare, c_sq, 0, 0, 0, 0, 0, 0, 1));
            }
        }
    }

    void Worker::getPseudoMoves(Nimorak::Game& game, MoveList& moves, bool onlyCaptures)
    {
        moves.clear();

        getPawnMoves(game, moves, onlyCaptures);
        getKnightMoves(game, moves, onlyCaptures);
        getKingMoves(game, moves, onlyCaptures);

        getSlidingMoves(game, moves, BISHOP, onlyCaptures);
        getSlidingMoves(game, moves, ROOK, onlyCaptures);
        getSlidingMoves(game, moves, QUEEN, onlyCaptures);

        if (!onlyCaptures) getCastleMoves(game, moves);
    }

    void Worker::getLegalMoves(Nimorak::Game& game, MoveList& moves, bool onlyCaptures)
    {
        moves.clear();
        
        getPseudoMoves(game, moves, onlyCaptures || game.config.moveGen.doOnlyCaptures);

        //if (!game.config.moveGen.doLegalMoveFiltering) return;

        int legalMoveCount = 0;

        for (int i = 0; i < moves.size(); i++)
        {
            Move move = moves[i];

            Board::makeMove(game, move, MAKE_MOVE_LIGHT);

            if (!Board::isKingInCheck(game, !game.turn))
            {
                moves[legalMoveCount] = move;

                legalMoveCount++;
            }

            Board::unmakeMove(game, MAKE_MOVE_LIGHT);

            continue;
        }

        moves.setsize(legalMoveCount);
    }
}