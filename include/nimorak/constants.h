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

#define WHITE_KINGSIDE 0x1
#define WHITE_QUEENSIDE 0x2
#define BLACK_KINGSIDE 0x4
#define BLACK_QUEENSIDE 0x8

#define PIECE_TYPE_MASK 0x07  // 00000111
#define PIECE_COLOR_MASK 0x08 // 00001000

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

/* Time controls */

#define INFINITE_TIME 1000000000

/* Evaluation */

#define PAWN_CENTER_CONTROL_BONUS 80
#define CENTER_CONTROL_ATTACK_BONUS 10

#define MATE_SCORE 32000
#define MATE_THRESHOLD (MATE_SCORE - 1000)

#endif