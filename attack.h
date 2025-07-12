#ifndef ATTACK_H
#define ATTACK_H

#include "nimorak.h"

void attack_generate_table(GameState *game, int color);

void attack_print_table(GameState *game, int color);

void attack_clear_table(GameState *game, int color);

#endif