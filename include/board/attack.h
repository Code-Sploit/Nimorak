#ifndef ATTACK_H
#define ATTACK_H

#include <stdbool.h>

#include <nimorak.h>

void attack_add_piece(Game *game, int color, int piece_type, int square);

void attack_remove_piece(Game *game, int color, int piece_type, int square);

void attack_generate_table(Game *game, int color);
void attack_print_table(Game *game, int color);

AttackTable attack_get_table(Game *game, int color);

void attack_generate_all(Game *game);

void attack_update_incremental(Game *game, Move move);

#endif
