#ifndef MAGIC_H
#define MAGIC_H

#include <nimorak.h>

Bitboard magic_get_bishop_attacks(int square, Bitboard occupancy);
Bitboard magic_get_rook_attacks(int square, Bitboard occupancy);

#endif
