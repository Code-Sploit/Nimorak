#include <board/movegen.h>
#include <board/attack.h>
#include <board/board.h>

#include <table/magic.h>

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

static inline bool attack_sliding_piece_line_intersects_square(Game *game, int slider_square, int target_square, int piece_type, Bitboard occupancy)
{
    if (!IS_SLIDING_PIECE(piece_type)) return false;
    
    AttackTable attacks = game->attack_tables_pc[piece_type][slider_square];
    
    return (attacks & (1ULL << target_square)) != 0;
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

void attack_update_incremental(Game *game, Move move)
{
    int from = GET_FROM(move);
    int to   = GET_TO(move);

    Bitboard occ = game->occupancy[BOTH];

    Piece moving_piece = board_get_square(game, to);
    int color = GET_COLOR(moving_piece);
    int opponent = color ^ 1;
    int moved_type = GET_TYPE(moving_piece);

    // --- 1) Remove origin attack contribution ---
    AttackTable old_att_from = game->attack_map[color][from];
    if (old_att_from) {
        game->attack_map_full[color] &= ~old_att_from;
        game->attack_map[color][from] = 0ULL;
    }

    // --- Remove enemy attack at 'to' ---
    AttackTable old_att_to_enemy = game->attack_map[opponent][to];
    if (old_att_to_enemy) {
        game->attack_map_full[opponent] &= ~old_att_to_enemy;
        game->attack_map[opponent][to] = 0ULL;
    }

    // --- 2) Capture removal ---
    int capture_sq = IS_ENPASSANT(move) ? (color == WHITE ? to-8 : to+8) : to;
    Piece captured_piece = board_get_square(game, capture_sq);
    if (captured_piece != EMPTY) {
        int cap_color = GET_COLOR(captured_piece);
        AttackTable old_att_cap = game->attack_map[cap_color][capture_sq];
        if (old_att_cap) {
            game->attack_map_full[cap_color] &= ~old_att_cap;
            game->attack_map[cap_color][capture_sq] = 0ULL;
        }
    }

    // --- 3) Recompute attacks for moved piece ---
    AttackTable new_att_to;
    if (IS_SLIDING_PIECE(moved_type))
        new_att_to = attack_generate_sliders(moved_type, to, occ);
    else
        new_att_to = (moved_type == PAWN) ? game->attack_tables_pc_pawn[color][to] 
                                          : game->attack_tables_pc[moved_type][to];

    game->attack_map[color][to] = new_att_to;
    if (new_att_to) game->attack_map_full[color] |= new_att_to;

    // --- 4) Sliding pieces affected by move using ray masks ---
    // Precompute sliding piece bitboards
    Bitboard sliders = board_get_sliding_pieces_bitboard(game, WHITE) |
                       board_get_sliding_pieces_bitboard(game, BLACK);

    while (sliders) {
        int sq = __builtin_ctzll(sliders);
        sliders &= sliders - 1;

        Piece p = board_get_square(game, sq);
        int pt = GET_TYPE(p);
        int c  = GET_COLOR(p);

        if (!IS_SLIDING_PIECE(pt)) continue;

        // Only update if move affects piece along precomputed rays
        Bitboard move_mask = (1ULL << from) | (1ULL << to);
        if (move_mask & game->attack_tables_pc[pt][sq]) {
            AttackTable old_att = game->attack_map[c][sq];
            if (old_att) game->attack_map_full[c] &= ~old_att;

            AttackTable recalculated = attack_generate_sliders(pt, sq, occ);
            game->attack_map[c][sq] = recalculated;

            if (recalculated) game->attack_map_full[c] |= recalculated;
        }
    }

    // --- 5) Pawns and knights (unrolled slightly) ---
    Bitboard piece_bb[2] = { game->board[color][PAWN], game->board[color][KNIGHT] };
    AttackTable *tables[2] = { game->attack_tables_pc_pawn[color], game->attack_tables_pc[KNIGHT] };

    AttackTable add_accum = 0ULL;
    AttackTable remove_accum = 0ULL;

    for (int i = 0; i < 2; i++) {
        Bitboard pieces = piece_bb[i];
        AttackTable *table = tables[i];

        while (pieces) {
            int sq = __builtin_ctzll(pieces);
            pieces &= pieces - 1;

            AttackTable *map_sq = &game->attack_map[color][sq];
            AttackTable new_att = table[sq];
            AttackTable old_att = *map_sq;

            *map_sq = new_att;

            if (old_att) remove_accum |= old_att;
            if (new_att) add_accum |= new_att;
        }
    }

    // Apply all changes in one memory write
    game->attack_map_full[color] &= ~remove_accum;
    game->attack_map_full[color] |= add_accum;
}


