#include <table/table.h>
#include <table/magic.h>

#include <stdio.h>

int KNIGHT_OFFSETS[8] = {17, 10, 6, 15, -6, -15, -17, -10};

void table_precompute_king_attacks(Game *game)
{
    if (!game) return;

    // 8 possible king moves (N, NE, E, SE, S, SW, W, NW)
    int offsets[8] = {8, 9, 1, -7, -8, -9, -1, 7};

    for (int square = 0; square < 64; square++)
    {
        Bitboard moves = 0ULL;
        int rank = square / 8;
        int file = square % 8;

        for (int i = 0; i < 8; i++)
        {
            int target = square + offsets[i];
            if (target < 0 || target >= 64)
                continue;

            int target_rank = target / 8;
            int target_file = target % 8;

            // King moves only to squares one step away in any direction, so
            // the difference in rank and file should be at most 1
            if (abs(target_rank - rank) <= 1 && abs(target_file - file) <= 1)
            {
                moves |= 1ULL << target;
            }
        }

        game->attack_tables_pc[KING][square] = moves;
    }
}

void table_precompute_knight_attacks(Game *game)
{
    if (!game) return;

    for (int square = 0; square < 64; square++)
    {
        int file = square % 8;
        Bitboard moves = 0ULL;

        for (int j = 0; j < 8; j++)
        {
            int offset = KNIGHT_OFFSETS[j];
            int target = square + offset;

            // Skip if target is off the board
            if (target < 0 || target >= 64) continue;

            // Prevent wrap-around
            if ((offset == 17 && file == 7) ||  // NNE
                (offset == 15 && file == 0) ||  // NNW
                (offset == 10 && file >= 6) ||  // ENE
                (offset == 6  && file <= 1) ||  // WNW
                (offset == -17 && file == 0) || // SSW
                (offset == -15 && file == 7) || // SSE
                (offset == -10 && file <= 1) || // WSW
                (offset == -6  && file >= 6))   // ESE
                continue;

            moves |= 1ULL << target;
        }

        game->attack_tables_pc[KNIGHT][square] = moves;
    }
}

void table_precompute_sliding_attacks(Game *game, int piece_type)
{
    if (!game) return;

    for (int square = 0; square < 64; square++)
    {
        Bitboard attacks = 0ULL;

        switch (piece_type)
        {
            case BISHOP:
                attacks = magic_get_bishop_attacks(square, 0ULL);
                break;
            case ROOK:
                attacks = magic_get_rook_attacks(square, 0ULL);
                break;
            case QUEEN:
                attacks = magic_get_bishop_attacks(square, 0ULL)
                        | magic_get_rook_attacks(square, 0ULL);
                break;
            case KING:
                attacks = game->attack_tables_pc[KING][square];
                break;
            default:
                continue; // Skip non-sliders
        }

        game->attack_tables_pc[piece_type][square] = attacks;
    }
}

void table_precompute_all_attacks(Game *game)
{
    if (!game) return;

    table_precompute_king_attacks(game);
    table_precompute_knight_attacks(game);
    table_precompute_sliding_attacks(game, BISHOP);
    table_precompute_sliding_attacks(game, ROOK);
    table_precompute_sliding_attacks(game, QUEEN);
    table_precompute_sliding_attacks(game, KING);
}
