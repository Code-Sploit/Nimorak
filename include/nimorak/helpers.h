#ifndef HELPERS_H
#define HELPERS_H

#define GET_FROM(m)    ((m) & FROM_MASK)
#define GET_TO(m)      (((m) & TO_MASK) >> 6)
#define GET_PROMO(m)   (((m) & PROMO_MASK) >> 12)

#define IS_CAPTURE(m)    (((m) & CAPTURE_MASK) != 0)
#define IS_PROMO(m)      (((m) & PROMO_FLAG) != 0)
#define IS_ENPASSANT(m)  (((m) & ENPASSANT) != 0)
#define IS_CHECK(m)      (((m) & CHECK) != 0)
#define IS_DOUBLE_PUSH(m) (((m) & DOUBLE_PUSH) != 0)
#define IS_CASTLE(m)     (((m) & CASTLE) != 0)

#define MOVE(from, to, promo, capture, promo_flag, ep, check, dp, castle) \
    (((from) & 0x3F) | \
    (((to) & 0x3F) << 6) | \
    (((promo) & 0x7) << 12) | \
    ((capture) ? CAPTURE_MASK : 0) | \
    ((promo_flag) ? PROMO_FLAG : 0) | \
    ((ep) ? ENPASSANT : 0) | \
    ((check) ? CHECK : 0) | \
    ((dp) ? DOUBLE_PUSH : 0) | \
    ((castle) ? CASTLE : 0))

#define MAKE_PIECE(type, color) ((type) | ((color) << 3))

#define GET_TYPE(piece) ((piece) & PIECE_TYPE_MASK)
#define GET_COLOR(piece) (((piece) & PIECE_COLOR_MASK) >> 3)

#define REMOVE_CASTLE_RIGHTS(rights, side) (rights &= ~side)

#define MIN(a, b) ((a < b) ? a : b)

static inline int pop_lsb(Bitboard *bb) {
    int sq = __builtin_ctzll(*bb);
    *bb &= *bb - 1;
    return sq;
}

static inline int clamp(int val, int min, int max)
{
    if (val < min) return min;
    if (val > max) return max;
    
    return val;
}

#endif