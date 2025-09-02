#pragma once

#include <tables/constants.hpp>
#include <tables/table.hpp>   // PrecomputedTables::AttackTable
#include <tables/helpers.hpp>

namespace Nimorak {
    class Game; // forward declaration
}

namespace Attack {
    class Worker {
    public:
        Bitboard attackMap[2][64];     
        Bitboard attackMapFull[2];     
        Bitboard attackMapIncludes;

        void generatePawns(Nimorak::Game& game, PieceColor color);
        void generateKnights(Nimorak::Game& game, PieceColor color);
        void generateKing(Nimorak::Game& game, PieceColor color);
        void generateSliding(Nimorak::Game& game, PieceColor color, PieceType type);

        void generateTable(Nimorak::Game& game, int side);
        void printTable(Nimorak::Game& game, int side);

        void generateAll(Nimorak::Game& game);
        void update(Nimorak::Game& game, Move move);

        bool isSquareAttackedBy(int square, int color);

        PrecomputedTables::AttackTable preComputed;
    };

} // namespace Attack