#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <nimorak.h>

#define NUM_PIECE_TYPES 12
#define NUM_SQUARES 64
#define NUM_CASTLING 16
#define NUM_ENPASSANT 8

ZobristHash zobrist_compute_hash(Game *game);

void zobrist_update_move(Game *game, Move move, State *old_state);

void zobrist_update_board(Game *game);
void zobrist_init();

#endif