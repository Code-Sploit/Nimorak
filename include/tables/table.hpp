#pragma once

#include <tables/constants.hpp>

namespace PrecomputedTables {

    class AttackTable {
    public:
        Bitboard pieces[7][64];      // Precomputed attacks for pieces
        Bitboard pawns[2][64];       // Precomputed pawn attacks
        uint8_t  castling[64][64];

        // Precompute all tables (call once at initialization)
        void preComputeKing();
        void preComputeKnight();
        void preComputeSliding(PieceType type);
        void preComputePawn(int color);
        void preComputeCastling();

        void preComputeAll();

        Bitboard getPawnAttacks(int color, int square);
        Bitboard getKnightAttacks(int square);
        Bitboard getSlidingAttacks(int square, PieceType type);
        Bitboard getKingAttacks(int square);
    };

} // namespace PrecomputedTables