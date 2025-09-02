#pragma once

#include <cstdint>
#include <tables/constants.hpp>
#include <tables/magic_bitboards.hpp>

namespace Magic {

    // ----------------------------
    // Bishop attacks
    // ----------------------------
    inline Bitboard getBishopAttacks(int square, Bitboard occupancy) {
        Bitboard blockers = occupancy & bishop_masks[square];
        std::size_t index = static_cast<std::size_t>((blockers * bishop_magics[square]) >> bishop_shifts[square]);
        return bishop_attack_tables[square][index];
    }

    // ----------------------------
    // Rook attacks
    // ----------------------------
    inline Bitboard getRookAttacks(int square, Bitboard occupancy) {
        Bitboard blockers = occupancy & rook_masks[square];
        std::size_t index = static_cast<std::size_t>((blockers * rook_magics[square]) >> rook_shifts[square]);
        return rook_attack_tables[square][index];
    }

    // ----------------------------
    // Queen attacks = Bishop + Rook
    // ----------------------------
    inline Bitboard getQueenAttacks(int square, Bitboard occupancy) {
        return getBishopAttacks(square, occupancy) | getRookAttacks(square, occupancy);
    }

} // namespace Magic