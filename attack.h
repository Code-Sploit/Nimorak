#ifndef ATTACK_H
#define ATTACK_H

#include "nimorak.h"

void generate_attack_tables(GameState *game, int color);

void print_attack_tables(GameState *game, int color);

void clear_attack_tables(GameState *game, int color);

#endif