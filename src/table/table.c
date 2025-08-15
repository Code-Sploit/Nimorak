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

void table_precompute_pawn_attacks(Game *game, int color)
{
    if (!game) return;

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

    for (int square = 0; square < 64; square++)
    {
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

        game->attack_tables_pc_pawn[color][square] = attacks;
    }
}

void table_precompute_castling_rights(Game *game) {
    for (int from = 0; from < 64; from++) {
        for (int to = 0; to < 64; to++) {
            uint8_t rights = CASTLING_ALL;

            // Moving king removes both rights for that color
            if (from == 4)   rights &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);
            if (from == 60)  rights &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);

            // Moving rooks removes that side’s rights
            if (from == 0)   rights &= ~WHITE_QUEENSIDE;
            if (from == 7)   rights &= ~WHITE_KINGSIDE;
            if (from == 56)  rights &= ~BLACK_QUEENSIDE;
            if (from == 63)  rights &= ~BLACK_KINGSIDE;

            // Capturing rook removes that side’s rights
            if (to == 0)     rights &= ~WHITE_QUEENSIDE;
            if (to == 7)     rights &= ~WHITE_KINGSIDE;
            if (to == 56)    rights &= ~BLACK_QUEENSIDE;
            if (to == 63)    rights &= ~BLACK_KINGSIDE;

            game->castling_rights_lookup[from][to] = rights;
        }
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
    table_precompute_pawn_attacks(game, WHITE);
    table_precompute_pawn_attacks(game, BLACK);
    table_precompute_castling_rights(game);
}
