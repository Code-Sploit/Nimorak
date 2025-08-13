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

static inline bool IS_SLIDING_PIECE(int piece_type)
{
    return (piece_type == BISHOP || piece_type == ROOK || piece_type == QUEEN);
}

static inline AttackTable generate_sliding_attacks(int piece_type, int square, Bitboard occupancy)
{
    switch (piece_type)
    {
        case BISHOP: return magic_get_bishop_attacks(square, occupancy);
        case ROOK:   return magic_get_rook_attacks(square, occupancy);
        case QUEEN:  return magic_get_bishop_attacks(square, occupancy) | magic_get_rook_attacks(square, occupancy);
        default:     return 0ULL;
    }
}

static inline bool sliding_piece_line_intersects_square(int slider_square, int target_square, int piece_type, Bitboard occupancy)
{
    if (!IS_SLIDING_PIECE(piece_type)) return false;
    AttackTable attacks = generate_sliding_attacks(piece_type, slider_square, occupancy);
    return (attacks & (1ULL << target_square)) != 0;
}

static inline Bitboard get_sliding_pieces_bitboard(Game *game, int color)
{
    // do NOT include king
    return game->board[color][BISHOP] | game->board[color][ROOK] | game->board[color][QUEEN];
}

void attack_update_incremental(Game *game, Move move)
{
    int from = GET_FROM(move);
    int to   = GET_TO(move);

    // ---------- Preconditions ----------
    // Board and occupancy must already reflect the move (i.e. piece removed from `from`, placed on `to`,
    // captured piece removed, en-passant captured pawn removed if applicable).
    // -----------------------------------

    Bitboard occ = game->occupancy[BOTH];

    // Moving piece is now on 'to'
    Piece moving_piece = board_get_square(game, to);
    int color = GET_COLOR(moving_piece);
    int opponent = color ^ 1;
    int moved_type = GET_TYPE(moving_piece);

    // --- 1) Remove contribution of the origin square 'from' (it is now empty) ---
    // Clear that origin's attack bitboard and remove its attacked squares from full map.
    AttackTable old_att_from = game->attack_map[color][from];
    if (old_att_from) {
        game->attack_map_full[color] &= ~old_att_from;
        game->attack_map[color][from] = 0ULL;
    }

    // Also clear enemy attack contribution at 'to' square (where piece just moved in)
    AttackTable old_att_to_enemy = game->attack_map[!color][to];
    if (old_att_to_enemy)
    {
        game->attack_map_full[!color] &= ~old_att_to_enemy;
        game->attack_map[!color][to] = 0ULL;
    }

    // --- 2) Handle capture removal (normal capture or en-passant) ---
    int capture_sq = to;
    if (IS_ENPASSANT(move)) {
        // If en-passant, captured pawn is behind the 'to' square
        capture_sq = (color == WHITE) ? (to - 8) : (to + 8);
    }

    Piece captured_piece = board_get_square(game, capture_sq);
    if (captured_piece != EMPTY) {
        int cap_color = GET_COLOR(captured_piece);
        AttackTable old_att_cap = game->attack_map[cap_color][capture_sq];
        if (old_att_cap) {
            game->attack_map_full[cap_color] &= ~old_att_cap;
            game->attack_map[cap_color][capture_sq] = 0ULL;
        }
    }

    // --- 3) Recompute attacks for the moved piece at its new 'to' square and add to full map ---
    AttackTable new_att_to = 0ULL;
    if (IS_SLIDING_PIECE(moved_type)) {
        new_att_to = generate_sliding_attacks(moved_type, to, occ);
    } else {
        if (moved_type == PAWN) {
            new_att_to = game->attack_tables_pc_pawn[color][to];
        } else {
            // knights, king, etc. use static precomputed masks
            new_att_to = game->attack_tables_pc[moved_type][to];
        }
    }

    // Store new attacks and add to full map
    game->attack_map[color][to] = new_att_to;
    if (new_att_to) game->attack_map_full[color] |= new_att_to;

    // --- 3.5) Recalculate attacks for pawns potentially affected ---

    // --- 4) Sliding pieces that might be affected by the move (open/close lines) ---
    // Only sliding pieces (rooks/bishops/queens) can have their attack set changed by blockers moving.
    Bitboard sliders = get_sliding_pieces_bitboard(game, WHITE) | get_sliding_pieces_bitboard(game, BLACK);

    // We'll iterate each sliding piece and only recalc if its attack ray intersects `from` or `to`
    while (sliders) {
        int sq = __builtin_ctzll(sliders);
        sliders &= sliders - 1;

        Piece p = board_get_square(game, sq);
        if (p == EMPTY) continue;                    // defensive
        int pt = GET_TYPE(p);
        int c  = GET_COLOR(p);

        // Only recalc for real sliding pieces
        if (!IS_SLIDING_PIECE(pt)) continue;

        // If the move changed a square on this piece's ray, we must recalc
        if (sliding_piece_line_intersects_square(sq, from, pt, occ)
         || sliding_piece_line_intersects_square(sq, to, pt, occ))
        {
            AttackTable old_att = game->attack_map[c][sq];
            if (old_att) game->attack_map_full[c] &= ~old_att;

            AttackTable recalculated = generate_sliding_attacks(pt, sq, occ);
            game->attack_map[c][sq] = recalculated;
            if (recalculated) game->attack_map_full[c] |= recalculated;
        }
    }

    Bitboard pawns = game->board[color][PAWN];
    while (pawns) {
        int sq = __builtin_ctzll(pawns);
        pawns &= pawns - 1;

        AttackTable old_att_pawn = game->attack_map[color][sq];
        if (old_att_pawn) game->attack_map_full[color] &= ~old_att_pawn;

        AttackTable new_att_pawn = game->attack_tables_pc_pawn[color][sq];
        game->attack_map[color][sq] = new_att_pawn;
        if (new_att_pawn) game->attack_map_full[color] |= new_att_pawn;
    }

    Bitboard knights = game->board[color][KNIGHT];
    while (knights) {
        int sq = __builtin_ctzll(knights);
        knights &= knights - 1;

        AttackTable old_att_knight = game->attack_map[color][sq];
        if (old_att_knight) game->attack_map_full[color] &= ~old_att_knight;

        AttackTable new_att_knight = game->attack_tables_pc[KNIGHT][sq];
        game->attack_map[color][sq] = new_att_knight;
        if (new_att_knight) game->attack_map_full[color] |= new_att_knight;
    }
}