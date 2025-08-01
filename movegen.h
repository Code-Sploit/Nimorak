#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "nimorak.h"

extern int BISHOP_OFFSETS[4];
extern int ROOK_OFFSETS[4];
extern int QUEEN_OFFSETS[8];
extern int KING_OFFSETS[8];

/* Offsets for pawns (inverted for black) */

extern int PAWN_OFFSETS[4];

/* Offsets for knights */

extern int KNIGHT_OFFSETS[8];

void movegen_generate_pawn_moves(Game *game);
void movegen_generate_knight_moves(Game *game);
void movegen_generate_sliding_moves(Game *game, int piece_type);

void movegen_generate_pseudo_moves(Game *game);
void movegen_generate_legal_moves(Game *game);

#endif
