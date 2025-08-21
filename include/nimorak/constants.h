#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

/* Type definitions */

typedef uint64_t Bitboard;
typedef uint64_t ZobristHash;
typedef uint64_t AttackTable;

typedef uint32_t Move;

typedef uint8_t CastlingRights;
typedef uint8_t Piece;

/* Mask definitions */

#define FROM_MASK     0x0000003F      // bits 0-5
#define TO_MASK       0x00000FC0      // bits 6-11
#define PROMO_MASK    0x00007000      // bits 12-14 (3 bits)

#define CAPTURE_MASK  0x00008000      // bit 15
#define PROMO_FLAG    0x00010000      // bit 16
#define ENPASSANT     0x00020000      // bit 17
#define CHECK         0x00040000      // bit 18
#define DOUBLE_PUSH   0x00080000      // bit 19
#define CASTLE        0x00100000      // bit 20

#define WHITE_KINGSIDE  (1 << 0)
#define WHITE_QUEENSIDE (1 << 1)
#define BLACK_KINGSIDE  (1 << 2)
#define BLACK_QUEENSIDE (1 << 3)
#define CASTLING_ALL    (WHITE_KINGSIDE | WHITE_QUEENSIDE | BLACK_KINGSIDE | BLACK_QUEENSIDE)

#define PIECE_TYPE_MASK 0x07  // 00000111
#define PIECE_COLOR_MASK 0x08 // 00001000

// Precomputed masks for files
#define FILE_A 0x0101010101010101ULL
#define FILE_H 0x8080808080808080ULL

// Precomputed masks for ranks
#define RANK_2 0x000000000000FF00ULL
#define RANK_7 0x00FF000000000000ULL

// Promotion ranks
#define RANK_8 0xFF00000000000000ULL
#define RANK_1 0x00000000000000FFULL

/* Piece types */

#define EMPTY   0
#define PAWN    1
#define KNIGHT  2
#define BISHOP  3
#define ROOK    4
#define QUEEN   5
#define KING    6
#define ALL_PIECES 7

/* Colors */

#define WHITE   0
#define BLACK   1
#define BOTH    2

/* Make & Unmake move types */

#define MAKE_MOVE_LIGHT 0
#define MAKE_MOVE_FULL  1

#define MAX_MOVES 256

/* Evaluation */

#define EVAL_PAWN_CENTER_CONTROL_BONUS 80
#define EVAL_CENTER_CONTROL_ATTACK_BONUS 10
#define EVAL_BISHOP_PAIR_BONUS 40

#define MATE_SCORE 32000
#define MATE_THRESHOLD (MATE_SCORE - 1000)

/* Search */

#define SEARCH_THINK_TIME_MARGIN 10

#define SEARCH_ORDERING_PROMO_BONUS 10000
#define SEARCH_ORDERING_CHECK_BONUS 5000
#define SEARCH_ORDERING_KILLER_MOVE1_SCORE 900000
#define SEARCH_ORDERING_KILLER_MOVE2_SCORE 800000

#endif