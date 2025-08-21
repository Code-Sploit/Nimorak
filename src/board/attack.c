#include <board/movegen.h>
#include <board/attack.h>
#include <board/board.h>

#include <table/magic.h>
#include <table/zobrist.h>

#include <nimorak.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static inline AttackTable attack_generate_sliders(int piece_type, int square, Bitboard occupancy)
{
    switch (piece_type)
    {
        case BISHOP: return magic_get_bishop_attacks(square, occupancy);
        case ROOK:   return magic_get_rook_attacks(square, occupancy);
        case QUEEN:  return magic_get_bishop_attacks(square, occupancy) | magic_get_rook_attacks(square, occupancy);
        default:     return 0ULL;
    }
}

void attack_generate_pawn(Game *game, int color)
{
    if (!game) return;

    Bitboard pawns = game->board[color][PAWN];

    while (pawns)
    {
        int square = __builtin_ctzll(pawns);

        AttackTable attacks = game->attack_tables_pc_pawn[color][square];

        game->attack_map[color][square] = attacks;

        game->attack_map_includes_square |= 1ULL << square;

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

        game->attack_map_includes_square |= 1ULL << square;

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

        game->attack_map_includes_square |= 1ULL << square;
    }
}

void attack_generate_table(Game *game, int color) {
    if (!game) return;

    AttackTable *attack_map = game->attack_map[color];
    
    memset(attack_map, 0, 64 * sizeof(AttackTable));

    game->attack_map_includes_square = 0ULL;

    attack_generate_pawn(game, color);
    attack_generate_knight(game, color);
    attack_generate_sliding(game, color, BISHOP);
    attack_generate_sliding(game, color, ROOK);
    attack_generate_sliding(game, color, QUEEN);
    attack_generate_sliding(game, color, KING);

    AttackTable full = 0ULL;
    
    Bitboard occ = game->attack_map_includes_square;
    
    while (occ) {
        int sq = __builtin_ctzll(occ);
        full |= attack_map[sq];
        occ &= occ - 1;
    }

    game->attack_map_full[color] = full;
}

void attack_generate_all(Game *game)
{
    // Local occupancy (both colors combined)
    Bitboard occ = game->occupancy[BOTH];

    // Clear attack maps
    memset(game->attack_map, 0, sizeof(game->attack_map));

    game->attack_map_full[WHITE] = 0ULL;
    game->attack_map_full[BLACK] = 0ULL;

    // Track all occupied squares
    game->attack_map_includes_square = occ;

    // Loop all occupied squares only once
    Bitboard temp = occ;

    while (temp)
    {
        int sq = __builtin_ctzll(temp);

        temp &= temp - 1;

        int piece = game->board_ghost[sq];  // Non‐empty guaranteed
        int color = GET_COLOR(piece);
        
        Bitboard attacks;

        switch (GET_TYPE(piece))
        {
            case PAWN:
                attacks = game->attack_tables_pc_pawn[color][sq];
                break;
            case KNIGHT:
                attacks = game->attack_tables_pc[KNIGHT][sq];
                break;
            case BISHOP:
                attacks = magic_get_bishop_attacks(sq, occ);
                break;
            case ROOK:
                attacks = magic_get_rook_attacks(sq, occ);
                break;
            case QUEEN:
                attacks = magic_get_bishop_attacks(sq, occ) |
                          magic_get_rook_attacks(sq, occ);
                break;
            case KING:
                attacks = game->attack_tables_pc[KING][sq];
                break;
            default:
                attacks = 0ULL; // Should never hit
                break;
        }

        // Store per‐square attacks
        game->attack_map[color][sq] = attacks;

        // Add to per‐color aggregate
        game->attack_map_full[color] |= attacks;
    }
}

void attack_print_table(Game *game, int color)
{
    if (!game) return;

    printf("\n");

    Bitboard attack_map = game->attack_map_full[color];

    for (int rank = 7; rank >= 0; rank--)
    {
        printf(" +---+---+---+---+---+---+---+---+\n ");

        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;

            if ((attack_map & (1ULL << square)) == 0)
            {
                printf("|   ");
            }
            else
            {
                printf("| X ");
            }
        }

        printf("| %d  ", rank + 1);

        printf("\n");
    }

    printf(" +---+---+---+---+---+---+---+---+\n ");

    printf("  a   b   c   d   e   f   g   h \n");

    printf("\nFEN: %s\n", board_generate_fen(game));
    printf("Zobrist Key: %s\n", zobrist_key_to_string(game->zobrist_key));
    printf("Checkers: '%s'\n", board_get_checkers(game));
    printf("\n");
}

static inline int attack_count_square(Game *game, int target, int color)
{
    AttackTable board = game->attack_map_full[color];

    int count = 0;

    while (board)
    {
        int square = __builtin_ctzll(board);

        AttackTable attacked_squares = game->attack_map[color][square];

        if ((attacked_squares & (1ULL << target)) != 0) count++;

        board &= board - 1;
    }

    return count;
}

void attack_update_incremental(Game *game, Move move)
{
    const int from = GET_FROM(move);
    const int to   = GET_TO(move);
    const Bitboard occ = game->occupancy[BOTH];

    const int color = GET_COLOR(board_get_square(game, to));
    const int opponent = color ^ 1;

    // Determine the moved piece type (handle promotion)
    int moved_type = GET_TYPE(board_get_square(game, to));
    if (IS_PROMO(move)) {
        moved_type = GET_PROMO(move); // QUEEN, ROOK, KNIGHT, BISHOP
    }

    // --- Clear old attacks for moving piece ---
    game->attack_map[color][from] = 0ULL;

    // --- Clear opponent's attacks from 'to' square ---
    game->attack_map[opponent][to] = 0ULL;

    // --- Handle en passant ---
    int capture_sq = to;
    if (IS_ENPASSANT(move)) {
        capture_sq = (color == WHITE) ? to - 8 : to + 8;
    }

    // --- Clear attacks of captured piece ---
    Piece captured_piece = board_get_square(game, capture_sq);
    if (GET_TYPE(captured_piece) != EMPTY) {
        int cap_color = GET_COLOR(captured_piece);
        game->attack_map[cap_color][capture_sq] = 0ULL;
    } else if (IS_ENPASSANT(move)) {
        // Clear attack map for en passant square even if empty
        game->attack_map[opponent][capture_sq] = 0ULL;
    }

    // --- Add new attacks for moved piece ---
    Bitboard new_att = 0ULL;
    switch (moved_type) {
        case BISHOP: new_att = magic_get_bishop_attacks(to, occ); break;
        case ROOK:   new_att = magic_get_rook_attacks(to, occ); break;
        case QUEEN:  new_att = magic_get_bishop_attacks(to, occ) | magic_get_rook_attacks(to, occ); break;
        case PAWN:   new_att = game->attack_tables_pc_pawn[color][to]; break;
        case KNIGHT: new_att = game->attack_tables_pc[KNIGHT][to]; break;
        case KING:   new_att = game->attack_tables_pc[KING][to]; break;
    }
    game->attack_map[color][to] = new_att;

    // --- Update sliders affected by from/to moves ---
    Bitboard slider_occ = game->board[WHITE][BISHOP] | game->board[WHITE][ROOK] | game->board[WHITE][QUEEN] |
                          game->board[BLACK][BISHOP] | game->board[BLACK][ROOK] | game->board[BLACK][QUEEN];
    Bitboard affected = slider_occ & (
        magic_get_bishop_attacks(from, occ) | magic_get_bishop_attacks(to, occ) |
        magic_get_rook_attacks(from, occ)   | magic_get_rook_attacks(to, occ)
    );

    while (affected) {
        int sq = __builtin_ctzll(affected);
        affected &= affected - 1;

        Piece p = board_get_square(game, sq);
        int c = GET_COLOR(p);
        int pt = GET_TYPE(p);

        if (pt == BISHOP)       new_att = magic_get_bishop_attacks(sq, occ);
        else if (pt == ROOK)    new_att = magic_get_rook_attacks(sq, occ);
        else if (pt == QUEEN)   new_att = magic_get_bishop_attacks(sq, occ) | magic_get_rook_attacks(sq, occ);
        else continue;

        game->attack_map[c][sq] = new_att;
    }

    // --- Update pawns, knights, king for moving color ---
    Bitboard pcs;

    pcs = game->board[color][PAWN];
    while (pcs) {
        int sq = __builtin_ctzll(pcs);
        pcs &= pcs - 1;
        game->attack_map[color][sq] = game->attack_tables_pc_pawn[color][sq];
    }

    pcs = game->board[color][KNIGHT];
    while (pcs) {
        int sq = __builtin_ctzll(pcs);
        pcs &= pcs - 1;
        game->attack_map[color][sq] = game->attack_tables_pc[KNIGHT][sq];
    }

    pcs = game->board[color][KING];
    if (pcs) {
        int sq = __builtin_ctzll(pcs);
        game->attack_map[color][sq] = game->attack_tables_pc[KING][sq];
    }

    // --- Recompute full attack map for both colors ---
    for (int c = 0; c < 2; c++) {
        game->attack_map_full[c] = 0ULL;
        for (int s = 0; s < 64; s++) {
            game->attack_map_full[c] |= game->attack_map[c][s];
        }
    }
}
