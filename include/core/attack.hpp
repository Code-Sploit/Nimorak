#pragma once

#include <tables/constants.hpp>
#include <tables/table.hpp>   // PrecomputedTables::AttackTable
#include <tables/helpers.hpp>

namespace Rune {
    class Game; // forward declaration
}

namespace Attack {
    class Worker {
    public:
        Bitboard attackMap[2][64];     
        Bitboard attackMapFull[2];     
        Bitboard attackMapIncludes;

        void generatePawns(Rune::Game& game, PieceColor color);
        void generateKnights(Rune::Game& game, PieceColor color);
        void generateKing(Rune::Game& game, PieceColor color);
        void generateSliding(Rune::Game& game, PieceColor color, PieceType type);

        void generateTable(Rune::Game& game, int side);
        void printTable(Rune::Game& game, int side);

        void generateAll(Rune::Game& game);
        void update(Rune::Game& game, Move move);

        bool isSquareAttackedBy(int square, int color);
        
        Bitboard getNewAttacksForMove(Rune::Game& game, Move move);
        Bitboard getAttackersForSquare(Rune::Game& game, int color, int square);

        PrecomputedTables::AttackTable preComputed;
    };

} // namespace Attack