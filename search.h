#ifndef SEARCH_H
#define SEARCH_H

#include "nimorak.h"
#include "board.h"

int search_position(GameState *game, int depth, int alpha, int beta);
Move find_best_move(GameState *game, int depth);

#endif