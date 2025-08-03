#ifndef MAGIC_H
#define MAGIC_H

#include <nimorak.h>

#define BISHOP_ATTACK_TABLE_SIZE 5248
#define ROOK_ATTACK_TABLE_SIZE 102400

extern Bitboard bishop_attack_table[BISHOP_ATTACK_TABLE_SIZE];
extern Bitboard rook_attack_table[ROOK_ATTACK_TABLE_SIZE];

Magic* generate_bishop_magics();
Magic* generate_rook_magics();

Bitboard magic_get_bishop_attacks(Game *game, int square, Bitboard occupancy);
Bitboard magic_get_rook_attacks(Game *game, int square, Bitboard occupancy);

void magic_best_bishops();

void magic_best_rooks();
#endif
