#ifndef CONSTANTS_H
#define CONSTANTS_H

#define QUEEN_SIDE 0
#define KING_SIDE 1

#define EMPTY 0

#define WHITE 0
#define BLACK 1

#define MAX_LEGAL_MOVES 256

enum {
    W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = -1, B_KNIGHT = -2, B_BISHOP = -3, B_ROOK = -4, B_QUEEN = -5, B_KING = -6
};

extern int KNIGHT_OFFSETS[];
extern int BISHOP_OFFSETS[];
extern int ROOK_OFFSETS[];
extern int QUEEN_OFFSETS[];
extern int KING_OFFSETS[];
extern int PAWN_OFFSETS[];

extern int pawn_pst[64];

extern int knight_pst[64];
extern int bishop_pst[64];
extern int rook_pst[64];

extern int queen_pst[64];

extern int king_pst[64];

#endif