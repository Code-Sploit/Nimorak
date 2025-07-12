#ifndef BOARD_H
#define BOARD_H

#include "nimorak.h"

void board_load_fen(GameState *game, const char *fen);

void board_movelist_clear(GameState *game);

void board_movelist_add(GameState *game, Move move);

void board_make_move(GameState *game, Move move);

void board_make_move_str(GameState* game, const char* move_str);

void board_clear(GameState *game);

void board_print(GameState *game);

#endif