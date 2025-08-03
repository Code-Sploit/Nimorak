#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <table/magic_bitboards.h>
#include <table/magic.h>

Bitboard magic_get_bishop_attacks(Game *game, int square, Bitboard occupancy) {
    Bitboard blockers = occupancy & bishop_masks[square];
    int index = (blockers * bishop_magics[square]) >> bishop_shifts[square];
    return bishop_attack_tables[square][index];
}

Bitboard magic_get_rook_attacks(Game *game, int square, Bitboard occupancy) {
    Bitboard blockers = occupancy & rook_masks[square];
    int index = (blockers * rook_magics[square]) >> rook_shifts[square];
    return rook_attack_tables[square][index];
}