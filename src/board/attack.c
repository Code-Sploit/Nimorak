#include <board/movegen.h>
#include <board/attack.h>
#include <board/board.h>

#include <table/magic.h>

#include <nimorak.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void attack_generate_pawn(Game *game, int color)
{
    if (!game) return;

    Bitboard pawns = game->board[color][PAWN];

    while (pawns)
    {
        int square = __builtin_ctzll(pawns);

        AttackTable attacks = game->attack_tables_pc_pawn[color][square];

        game->attack_map[color][square] = attacks;

        pawns &= pawns - 1;
    }
}

void attack_generate_knight(Game *game, int color)
{
    if (!game) return;

    Bitboard knights = game->board[color][KNIGHT];

    while (knights)
    {
        int square = __builtin_ctzll(knights);

        AttackTable attacks = game->attack_tables_pc[KNIGHT][square];

        game->attack_map[color][square] = attacks;

        knights &= knights - 1;
    }
}

void attack_generate_sliding(Game *game, int color, int piece_type) {
    if (!game) return;

    Bitboard sliders = game->board[color][piece_type];
    Bitboard occupancy = game->occupancy[BOTH];
    AttackTable *attack_map = game->attack_map[color];

    while (sliders) {
        int square = __builtin_ctzll(sliders);
        sliders &= sliders - 1;

        Bitboard attacks;
        if (piece_type == BISHOP)
            attacks = magic_get_bishop_attacks(square, occupancy);
        else if (piece_type == ROOK)
            attacks = magic_get_rook_attacks(square, occupancy);
        else if (piece_type == QUEEN)
            attacks = magic_get_bishop_attacks(square, occupancy) | magic_get_rook_attacks(square, occupancy);
        else if (piece_type == KING)
            attacks = game->attack_tables_pc[KING][square];
        else
            continue;

        attack_map[square] = attacks;
    }
}

void attack_generate_table(Game *game, int color) {
    if (!game) return;

    AttackTable *attack_map = game->attack_map[color];
    memset(attack_map, 0, 64 * sizeof(AttackTable));

    attack_generate_pawn(game, color);
    attack_generate_knight(game, color);
    attack_generate_sliding(game, color, BISHOP);
    attack_generate_sliding(game, color, ROOK);
    attack_generate_sliding(game, color, QUEEN);
    attack_generate_sliding(game, color, KING);

    // Rebuild full attack bitboard once per color after all updates
    AttackTable full = 0ULL;
    
    for (int i = 0; i < 64; i++) {
        full |= attack_map[i];
    }
    game->attack_map_full[color] = full;
}

void attack_print_table(Game *game, int color)
{
    uint64_t bitboard = game->attack_map_full[color];

    printf("\nAttack Table for %s:\n\n", color == WHITE ? "WHITE" : "BLACK");

    for (int rank = 7; rank >= 0; rank--)
    {
        printf("%d  ", rank + 1); // Print rank numbers (1-8)

        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;

            if ((bitboard >> square) & 1ULL)
                printf("x ");
            else
                printf(". ");
        }
        printf("\n");
    }

    printf("\n   a b c d e f g h\n\n"); // Print file letters
}

AttackTable attack_generate_single_pawn(Game *game, int square, int color)
{
    if (!game) return 0;

    int pawn_capture_left;
    int pawn_capture_right;

    if (color == WHITE)
    {
        pawn_capture_left = 7;
        pawn_capture_right = 9;
    }
    else
    {
        pawn_capture_left = -9;
        pawn_capture_right = -7;
    }

    int file   = square % 8;

    AttackTable attacks = 0ULL;

    // Capture left (only if not on file A)
    if (file > 0)
    {
        int target = square + pawn_capture_left;

        if (target >= 0 && target < 64)
        {
            attacks |= (1ULL << target);
        }
    }

    // Capture right (only if not on file H)
    if (file < 7)
    {
        int target = square + pawn_capture_right;

        if (target >= 0 && target < 64)
        {
            attacks |= (1ULL << target);
        }
    }

    return attacks;
}

AttackTable attack_generate_single_knight(Game *game, int square)
{
    if (!game) return 0;

    AttackTable attacks = 0ULL;

    int file = square % 8;
    int rank = square / 8;

    for (int j = 0; j < 8; j++)
    {
        int offset = KNIGHT_OFFSETS[j];
        int target = square + offset;

        if (target < 0 || target >= 64) continue;

        int target_file = target % 8;
        int target_rank = target / 8;

        // Ensure the knight didn't wrap around the board
        if (abs(target_file - file) > 2 || abs(target_rank - rank) > 2) continue;

        attacks |= (1ULL << target);
    }

    return attacks;
}

Bitboard attack_generate_single_sliding(Game *game, int square, int piece_type)
{
    if (!game) return 0;

    switch (piece_type)
    {
        case BISHOP:
            return magic_get_bishop_attacks(square, game->occupancy[BOTH]);

        case ROOK:
            return magic_get_rook_attacks(square, game->occupancy[BOTH]);

        case QUEEN:
            return magic_get_bishop_attacks(square, game->occupancy[BOTH]) |
                   magic_get_rook_attacks(square, game->occupancy[BOTH]);

        case KING:
            return game->attack_tables_pc[KING][square];

        default:
            return 0ULL;
    }
}

