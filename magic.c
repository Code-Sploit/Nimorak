#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "magic.h"

static inline int popcount(uint64_t bb) {
    int count = 0;
    while (bb) {
        bb &= bb - 1;
        count++;
    }
    return count;
}

static inline int to_square(int rank, int file) {
    return rank * 8 + file;
}

static inline int to_rank(int sq) {
    return sq / 8;
}

static inline int to_file(int sq) {
    return sq % 8;
}

static inline int in_bounds(int rank, int file) {
    return rank >= 0 && rank < 8 && file >= 0 && file < 8;
}

// Generate blocker mask for bishop
uint64_t bishop_mask(int square)
{
    uint64_t mask = 0ULL;
    int rank = to_rank(square);
    int file = to_file(square);
    int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (int d = 0; d < 4; d++) {
        int r = rank + directions[d][0];
        int f = file + directions[d][1];
        while (in_bounds(r, f)) {
            if (r == 0 || r == 7 || f == 0 || f == 7) break; // exclude edge
            mask |= 1ULL << to_square(r, f);
            r += directions[d][0];
            f += directions[d][1];
        }
    }
    return mask;
}

uint64_t rook_mask(int square)
{
    uint64_t mask = 0ULL;

    int rank = to_rank(square);
    int file = to_file(square);

    int directions[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

    for (int d = 0; d < 4; d++)
    {
        int r = rank + directions[d][0];
        int f = file + directions[d][1];

        while (in_bounds(r, f))
        {
            mask |= 1ULL << to_square(r, f);

            r += directions[d][0];
            f += directions[d][1];
        }
    }

    return mask;
}

// Generate actual bishop attacks (ignores own pieces, slides until blocker)
uint64_t bishop_attacks(int square, uint64_t blockers)
{
    uint64_t attacks = 0ULL;
    int rank = to_rank(square);
    int file = to_file(square);
    int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (int d = 0; d < 4; d++) {
        int r = rank + directions[d][0];
        int f = file + directions[d][1];
        while (in_bounds(r, f)) {
            int sq = to_square(r, f);
            attacks |= 1ULL << sq;
            if (blockers & (1ULL << sq)) break;
            r += directions[d][0];
            f += directions[d][1];
        }
    }
    return attacks;
}

uint64_t rook_attacks(int square, uint64_t blockers)
{
    uint64_t attacks = 0ULL;

    int rank = to_rank(square);
    int file = to_file(square);

    int directions[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

    for (int d = 0; d < 4; d++) {
        int r = rank + directions[d][0];
        int f = file + directions[d][1];
        while (in_bounds(r, f)) {
            int sq = to_square(r, f);
            attacks |= 1ULL << sq;
            if (blockers & (1ULL << sq)) break;
            r += directions[d][0];
            f += directions[d][1];
        }
    }
    return attacks;
}

// Convert an index to a blocker board based on the mask bits
uint64_t index_to_blockers(int index, uint64_t mask)
{
    uint64_t blockers = 0ULL;
    int bit = 0;
    for (int i = 0; i < 64; i++) {
        if (mask & (1ULL << i)) {
            if (index & (1 << bit)) {
                blockers |= (1ULL << i);
            }
            bit++;
        }
    }
    return blockers;
}

// Random 64-bit candidate for magic numbers
uint64_t random_magic_candidate()
{
    return ((uint64_t) rand() & 0xFFFF) |
           (((uint64_t) rand() & 0xFFFF) << 16) |
           (((uint64_t) rand() & 0xFFFF) << 32) |
           (((uint64_t) rand() & 0xFFFF) << 48);
}

// Try to find a collision-free magic number

uint64_t set_occupancy(int index, uint64_t mask) {
    uint64_t result = 0ULL;
    int bit_index = 0;

    for (int square = 0; square < 64; square++) {
        if (mask & (1ULL << square)) {
            if (index & (1 << bit_index))
                result |= (1ULL << square);
            bit_index++;
        }
    }

    return result;
}

uint64_t bishop_attacks_on_the_fly(int square, uint64_t blockers)
{
    uint64_t attacks = 0ULL;
    int rank = to_rank(square);
    int file = to_file(square);

    int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (int d = 0; d < 4; d++)
    {
        int r = rank + directions[d][0];
        int f = file + directions[d][1];

        while (in_bounds(r, f))
        {
            int sq = to_square(r, f);
            attacks |= (1ULL << sq);

            if (blockers & (1ULL << sq))
                break;

            r += directions[d][0];
            f += directions[d][1];
        }
    }

    return attacks;
}

uint64_t rook_attacks_on_the_fly(int square, uint64_t blockers)
{
    uint64_t attacks = 0ULL;
    int rank = to_rank(square);
    int file = to_file(square);

    int directions[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

    for (int d = 0; d < 4; d++)
    {
        int r = rank + directions[d][0];
        int f = file + directions[d][1];

        while (in_bounds(r, f))
        {
            int sq = to_square(r, f);
            attacks |= (1ULL << sq);

            if (blockers & (1ULL << sq))
                break;

            r += directions[d][0];
            f += directions[d][1];
        }
    }

    return attacks;
}

uint64_t find_magic(int square, int relevant_bits, uint64_t mask, uint64_t* table, int piece_type)
{
    int size = 1 << relevant_bits;
    uint64_t* used = calloc(size, sizeof(uint64_t));
    int attempts = 0;

    while (1) {
        attempts++;

        if (attempts % 1000 == 0) {
            //printf("Square %d (%s): Attempt %d...\n", square, piece_type == ROOK ? "Rook" : "Bishop", attempts);
            fflush(stdout); // Ensure output is not buffered
        }

        uint64_t magic = random_magic_candidate() & random_magic_candidate() & random_magic_candidate();

        memset(used, 0, size * sizeof(uint64_t));
        int fail = 0;

        for (int index = 0; index < size; index++) {
            uint64_t blockers = index_to_blockers(index, mask);
            uint64_t attack = (piece_type == BISHOP) ? bishop_attacks(square, blockers) : rook_attacks(square, blockers);
            uint64_t magic_index = (blockers * magic) >> (64 - relevant_bits);

            if (used[magic_index] == 0ULL)
                used[magic_index] = attack;
            else if (used[magic_index] != attack) {
                fail = 1;
                break;
            }
        }

        if (!fail) {
            for (int index = 0; index < size; index++) {
                uint64_t blockers = index_to_blockers(index, mask);
                uint64_t attack = (piece_type == BISHOP) ? bishop_attacks(square, blockers) : rook_attacks(square, blockers);
                uint64_t magic_index = (blockers * magic) >> (64 - relevant_bits);
                table[magic_index] = attack;
            }
            //printf("✅ Square %d (%s): Found magic after %d attempts\n", square, piece_type == ROOK ? "Rook" : "Bishop", attempts);
            fflush(stdout);
            free(used);
            return magic;
        }
    }
}

Magic* generate_bishop_magics()
{
    Magic* magics = malloc(sizeof(Magic) * 64);
    if (!magics) return NULL;

    for (int square = 0; square < 64; square++) {
        Magic* m = &magics[square];
        m->mask = bishop_mask(square);
        int relevant_bits = popcount(m->mask);
        m->shift = 64 - relevant_bits;
        int table_size = 1 << relevant_bits;
        m->table = calloc(table_size, sizeof(uint64_t));

        int permutations = 1 << relevant_bits;

        for (int i = 0; i < permutations; i++)
        {
            uint64_t blockers = set_occupancy(i, m->mask);
            uint64_t attacks = bishop_attacks_on_the_fly(square, blockers);
            int index = (blockers * m->magic) >> m->shift;

            m->table[index] = attacks;
        }


        if (!m->table) {
            for (int i = 0; i < square; i++) free(magics[i].table);
            free(magics);
            return NULL;
        }

        m->magic = find_magic(square, relevant_bits, m->mask, m->table, BISHOP);
    }

    return magics;
}

Magic* generate_rook_magics()
{
    Magic* magics = malloc(sizeof(Magic) * 64);
    if (!magics) return NULL;

    for (int square = 0; square < 64; square++) {
        Magic* m = &magics[square];
        m->mask = rook_mask(square);
        int relevant_bits = popcount(m->mask);
        m->shift = 64 - relevant_bits;
        int table_size = 1 << relevant_bits;
        m->table = calloc(table_size, sizeof(uint64_t));
        if (!m->table) {
            for (int i = 0; i < square; i++) free(magics[i].table);
            free(magics);
            return NULL;
        }

        m->magic = find_magic(square, relevant_bits, m->mask, m->table, ROOK);

        // No need to fill table here; find_magic already did it
    }

    return magics;
}

Bitboard magic_get_bishop_attacks(Game *game, int square, Bitboard occupancy)
{
    uint64_t blockers = occupancy & game->bishop_magics[square].mask;
    int index = (blockers * game->bishop_magics[square].magic) >> game->bishop_magics[square].shift;
    return game->bishop_magics[square].table[index];
}

Bitboard magic_get_rook_attacks(Game *game, int square, Bitboard occupancy)
{
    uint64_t blockers = occupancy & game->rook_magics[square].mask;
    int index = (blockers * game->rook_magics[square].magic) >> game->rook_magics[square].shift;
    return game->rook_magics[square].table[index];
}
