#pragma once

#include <tables/constants.hpp>

namespace Helpers {

    // Bitboard / move helpers
    inline int get_from(Move m)       { return m & FROM_MASK; }
    inline int get_to(Move m)         { return (m & TO_MASK) >> 6; }
    inline int get_promo(Move m)      { return (m & PROMO_MASK) >> 12; }
    inline int get_color(Piece p)     { return (p & PIECE_COLOR_MASK) >> 3; }
    inline int get_type(Piece p)      { return (p & PIECE_TYPE_MASK); }

    inline bool is_capture(Move m)    { return (m & CAPTURE_MASK) != 0; }
    inline bool is_promo(Move m)      { return (m & PROMO_FLAG) != 0; }
    inline bool is_enpassant(Move m)  { return (m & ENPASSANT) != 0; }
    inline bool is_check(Move m)      { return (m & CHECK) != 0; }
    inline bool is_double_push(Move m){ return (m & DOUBLE_PUSH) != 0; }
    inline bool is_castle(Move m)     { return (m & CASTLE) != 0; }

    // Min / Max templates
    template<typename T>
    inline T min(T a, T b) { return (a < b) ? a : b; }

    template<typename T>
    inline T max(T a, T b) { return (a > b) ? a : b; }

    // Piece helpers
    inline int make_piece(int type, int color) { return type | (color << 3); }

    // Chess helpers (to be implemented elsewhere if complex)
    bool is_sliding_piece(int type);

    inline int rank_of(int square) { return square / 8; }
    inline int file_of(int square) { return square % 8; }

    int pop_lsb(Bitboard *bitboard);

    inline int clamp(int val, int min_val, int max_val) {
        if (val < min_val) return min_val;
        if (val > max_val) return max_val;
        return val;
    }

    inline Move move(int from, int to, int promo, int capture, int promo_flag, int ep, int check, int dp, int castle)
    {
        return (((from) & 0x3F) | \
                (((to) & 0x3F) << 6) | \
                (((promo) & 0x7) << 12) | \
                ((capture) ? CAPTURE_MASK : 0) | \
                ((promo_flag) ? PROMO_FLAG : 0) | \
                ((ep) ? ENPASSANT : 0) | \
                ((check) ? CHECK : 0) | \
                ((dp) ? DOUBLE_PUSH : 0) | \
                ((castle) ? CASTLE : 0));
    }

} // namespace Helpers