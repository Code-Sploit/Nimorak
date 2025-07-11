#ifndef BOARD_H
#define BOARD_H

#include "nimorak.h"

void load_fen(GameState *game, const char *fen);

void clear_move_list(GameState *game);

void add_move(GameState *game, Move move);

void clone_game_state(GameState *dest, const GameState *src);

void make_move(GameState *game, Move move);

void make_move_str(GameState* game, const char* move_str);

#endif