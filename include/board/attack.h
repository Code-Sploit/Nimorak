#ifndef ATTACK_H
#define ATTACK_H

#include <stdbool.h>

#include <nimorak.h>

void attack_add_piece(Game *game, int color, int piece_type, int square);

void attack_remove_piece(Game *game, int color, int piece_type, int square);

void attack_generate_table(Game *game, int color);
void attack_print_table(Game *game, int color);

AttackTable attack_get_table(Game *game, int color);

Bitboard attack_generate_single_pawn(Game *game, int square, int color);
Bitboard attack_generate_single_knight(Game *game, int square);
Bitboard attack_generate_single_sliding(Game *game, int square, int piece_type);

void attack_update_after_move(Game *game, Move move);

void attack_update_sliders_for_square(Game *game, int square);

#endif
