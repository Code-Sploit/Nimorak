#include <stdio.h>
#include <stdint.h>
#include "magic_bitboards.h"

typedef uint64_t Bitboard;

Bitboard get_bishop_attacks(int square, Bitboard occupancy) {
    Bitboard blockers = occupancy & bishop_masks[square];
    int index = (blockers * bishop_magics[square]) >> bishop_shifts[square];
    return bishop_attack_tables[square][index];
}

Bitboard get_rook_attacks(int square, Bitboard occupancy) {
    Bitboard blockers = occupancy & rook_masks[square];
    int index = (blockers * rook_magics[square]) >> rook_shifts[square];
    return rook_attack_tables[square][index];
}

int main() {
    // Example usage:
    int square = 36; // e4
    Bitboard occupancy = 0x0000001008000000ULL; // some blocker example

    Bitboard bishop_attack = get_bishop_attacks(square, occupancy);
    Bitboard rook_attack = get_rook_attacks(square, occupancy);

    printf("Bishop attacks from %d: 0x%016llx\n", square, (unsigned long long)bishop_attack);
    printf("Rook attacks from %d:   0x%016llx\n", square, (unsigned long long)rook_attack);

    return 0;
}
