#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include "nimorak.h"
#include "zobrist.h"

#include <stdbool.h>

#define TT_EXACT 0
#define TT_ALPHA 1
#define TT_BETA  2

bool tt_probe(Game *game, ZobristHash key, int depth, int alpha, int beta, int *out_score);
void tt_store(Game *game, ZobristHash key, int depth, int eval, int flag, Move best_move);
void tt_clear(Game *game);

#endif