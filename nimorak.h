#ifndef NIMORAK_H
#define NIMORAK_H

#include <stdint.h>

/* A move is defined as a 32-bit integer 
    -> Bits 0-5 are the square the move is from
    -> Bits 6-11 are the square the move is to
    -> Bits 12-13 is the promotion piece type
    -> Bits 14-19 are flag bits
        -> Bit 14 is capture flag
        -> Bit 15 is promotion flag
        -> Bit 16 is enpassant flag
        -> Bit 17 is check flag
        -> Bit 18 is double pawn push flag
        -> Bit 19 is castle flag
*/

typedef uint32_t Move;

/* A piece is defined as a 8-bit integer
    -> Bits 0-2 are piece type
    -> Bit 3 is the piece color
    -> Bits 4-7 are optional flag bits
*/

typedef uint8_t Piece;

/* Bit masks for moves */

// Masks for each field
#define FROM_MASK     0x0000003F      // bits 0-5
#define TO_MASK       0x00000FC0      // bits 6-11
#define PROMO_MASK    0x00007000      // bits 12-14 (3 bits)

#define CAPTURE_MASK  0x00008000      // bit 15
#define PROMO_FLAG    0x00010000      // bit 16
#define ENPASSANT     0x00020000      // bit 17
#define CHECK         0x00040000      // bit 18
#define DOUBLE_PUSH   0x00080000      // bit 19
#define CASTLE        0x00100000      // bit 20

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

/* Bit masks for pieces */

#define PIECE_TYPE_MASK 0x07  // 00000111
#define PIECE_COLOR_MASK 0x08 // 00001000

// Piece types
#define EMPTY   0
#define PAWN    1
#define KNIGHT  2
#define BISHOP  3
#define ROOK    4
#define QUEEN   5
#define KING    6
#define ALL_PIECES 7

// Colors
#define WHITE   0
#define BLACK   1
#define BOTH    2

// Create piece
#define MAKE_PIECE(type, color) ((type) | ((color) << 3))

// Extractors
#define GET_TYPE(piece) ((piece) & PIECE_TYPE_MASK)
#define GET_COLOR(piece) (((piece) & PIECE_COLOR_MASK) >> 3)

/* Castling Rights is defined as an 8-bit integer */

typedef uint8_t CastlingRights;

#define WHITE_KINGSIDE 0x1
#define WHITE_QUEENSIDE 0x2
#define BLACK_KINGSIDE 0x4
#define BLACK_QUEENSIDE 0x8

#define REMOVE_CASTLE_RIGHTS(rights, side) (rights &= ~side)

/* Attack Table is defined as an 64-bit integer */

typedef uint64_t AttackTable;

/* Bitboard is a 64-bit integer */

typedef uint64_t Bitboard;

typedef struct {
    int count;

    Move moves[256];
} MoveList;

/* History for make_move and unmake_move */
typedef struct {
    CastlingRights castling_rights;
    Piece captured_piece;
    Move move;

    Bitboard attack_map_full[2];
    Bitboard attack_map[2][64];

    int enpassant_square;
} State;

typedef struct {
    uint64_t mask;
    uint64_t magic;

    uint64_t *table;

    int shift;
} Magic;

typedef struct {
    int turn;
    int enpassant_square;
    int history_count;
    
    CastlingRights castling_rights;

    Bitboard board[3][7];
    Bitboard occupancy[3];
    Bitboard attack_map[2][64];
    Bitboard attack_map_full[2];

    AttackTable attack_tables_pc[7][64];

    MoveList *movelist;

    State history[4096];

    Piece board_ghost[64];

    Magic *bishop_magics;
    Magic *rook_magics;
} Game;

Game *game_new();

void game_del(Game *game);

#endif
