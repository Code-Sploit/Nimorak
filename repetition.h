#ifndef REPETITION_H
#define REPETITION_H

#include "zobrist.h"
#include "nimorak.h"

void repetition_push(Game *game, ZobristHash hash);
void repetition_pop(Game *game);

int repetition_check_for_threefold(Game *game, ZobristHash hash);

#endif