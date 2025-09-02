#pragma once

#include <string>
#include <core/nimorak.hpp>
#include <tables/constants.hpp>
#include <tables/helpers.hpp>
#include <core/attack.hpp>

namespace Nimorak {
    class Game; // forward declaration
}

namespace Board {
    // ----------------------------
    // FEN & Board helpers
    // ----------------------------
    std::string generateFen(Nimorak::Game& game);
    void loadFen(Nimorak::Game& game, const std::string fenString);

    std::string getCheckers(Nimorak::Game& game);
    void print(Nimorak::Game& game);

    bool isOnRank(int square, int rank);
    bool isOnFile(int square, int file);

    std::string squareToName(int square);
    std::string moveToString(Move move);
    Move parseMove(Nimorak::Game& game, const std::string& moveStr);

    inline void setSquare(Nimorak::Game& game, int square, Piece piece);

    // ----------------------------
    // Move legality & castling
    // ----------------------------
    void makeMove(Nimorak::Game& game, Move move, int callType);
    void unmakeMove(Nimorak::Game& game, int callType);

    void makeNullMove(Nimorak::Game& game);
    void unmakeNullMove(Nimorak::Game& game);

    bool hasCastlingRights(Nimorak::Game& game, int side);
    bool hasCastlingRightsSide(Nimorak::Game& game, int side);

    bool isSameLine(int from, int to, int offset);
    int findKing(Nimorak::Game& game, int color);
    bool isKingInCheck(Nimorak::Game& game, int color);
    bool moveGivesCheck(Nimorak::Game& game, Move move);

    Bitboard getSlidingPiecesBitboard(Nimorak::Game& game, int color);

} // namespace Board