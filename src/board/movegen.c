#include <board/movegen.h>
#include <board/attack.h>
#include <board/board.h>

#include <table/magic.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int BISHOP_OFFSETS[4] = {-7, -9, 7, 9};
int ROOK_OFFSETS[4] = {-1, -8, 1, 8};
int QUEEN_OFFSETS[8] = {-7, -9, -1, -8, 7, 9, 1, 8};
int KING_OFFSETS[8] = {-7, -9, -1, -8, 7, 9, 1, 8};

int PAWN_OFFSETS[4] = {8, 16, 7, 9};

static inline void movegen_add_move(Game *game, Move move, MoveList *moves)
{
    if (!game) return;

    moves->moves[moves->count] = move;
    moves->count++;
}

static inline void movegen_add_promotion_moves(Game *game, int from, int to, int is_capture, MoveList *moves)
{
    movegen_add_move(game, MOVE(from, to, QUEEN, is_capture, 1, 0, 0, 0, 0), moves);
    movegen_add_move(game, MOVE(from, to, ROOK, is_capture, 1, 0, 0, 0, 0), moves);
    movegen_add_move(game, MOVE(from, to, BISHOP, is_capture, 1, 0, 0, 0, 0), moves);
    movegen_add_move(game, MOVE(from, to, KNIGHT, is_capture, 1, 0, 0, 0, 0), moves);
}

void movegen_generate_pawn_moves(Game *game, MoveList *moves, int only_captures)
{
    const int color = game->turn;
    const Bitboard own_occ   = game->occupancy[color];
    const Bitboard opp_occ   = game->occupancy[!color];
    const Bitboard all_occ   = own_occ | opp_occ;

    Bitboard pawns = game->board[color][PAWN];

    // Direction and special rank masks
    int push_dir = (color == WHITE) ? 8 : -8;
    int dbl_push_dir = (color == WHITE) ? 16 : -16;
    Bitboard promote_mask = (color == WHITE) ? RANK_8 : RANK_1;
    Bitboard start_rank_mask = (color == WHITE) ? RANK_2 : RANK_7;

    while (pawns) {
        int from = pop_lsb(&pawns);
        Bitboard from_bb = 1ULL << from;

        // Quiet moves
        if (!only_captures) {
            int to_sq = from + push_dir;
            Bitboard to_bb = 1ULL << to_sq;

            if (!(to_bb & all_occ)) {
                if (to_bb & promote_mask) {
                    movegen_add_promotion_moves(game, from, to_sq, 0, moves);
                } else {
                    movegen_add_move(game, MOVE(from, to_sq, 0, 0, 0, 0, 0, 0, 0), moves);

                    // Double push
                    if (from_bb & start_rank_mask) {
                        int to_sq2 = from + dbl_push_dir;
                        Bitboard to_bb2 = 1ULL << to_sq2;
                        if (!(to_bb2 & all_occ)) {
                            movegen_add_move(game, MOVE(from, to_sq2, 0, 0, 0, 0, 0, 1, 0), moves);
                        }
                    }
                }
            }
        }

        // Captures
        Bitboard left_captures, right_captures;
        if (color == WHITE) {
            left_captures  = (from_bb & ~FILE_A) << 7;
            right_captures = (from_bb & ~FILE_H) << 9;
        } else {
            left_captures  = (from_bb & ~FILE_A) >> 9;
            right_captures = (from_bb & ~FILE_H) >> 7;
        }

        Bitboard attacks = (left_captures | right_captures) & opp_occ;

        while (attacks) {
            int to_sq = pop_lsb(&attacks);
            if ((1ULL << to_sq) & promote_mask)
                movegen_add_promotion_moves(game, from, to_sq, 1, moves);
            else
                movegen_add_move(game, MOVE(from, to_sq, 0, 1, 0, 0, 0, 0, 0), moves);
        }

        // En passant
        if (game->enpassant_square != -1) {
            Bitboard ep_bb = 1ULL << game->enpassant_square;
            if (left_captures & ep_bb || right_captures & ep_bb) {
                movegen_add_move(game, MOVE(from, game->enpassant_square, 0, 1, 0, 1, 0, 0, 0), moves);
            }
        }
    }
}

void movegen_generate_knight_moves(Game *game, MoveList *moves, int only_captures)
{
    const int color = game->turn;
    const Bitboard knights = game->board[color][KNIGHT];
    const Bitboard friendly = game->occupancy[color];
    const Bitboard opp_occ = game->occupancy[!color];

    Bitboard k = knights;

    while (k)
    {
        int from = pop_lsb(&k);

        // Remove friendly pieces from attack set immediately
        Bitboard attacks = game->attack_tables_pc[KNIGHT][from] & ~friendly;

        if (only_captures) {
            // Keep only opponent pieces
            attacks &= opp_occ;
            while (attacks) {
                int to = pop_lsb(&attacks);
                movegen_add_move(game, MOVE(from, to, 0, 1, 0, 0, 0, 0, 0), moves);
            }
        } else {
            while (attacks) {
                int to = pop_lsb(&attacks);
                int is_capture = (opp_occ >> to) & 1ULL;
                movegen_add_move(game, MOVE(from, to, 0, is_capture, 0, 0, 0, 0, 0), moves);
            }
        }
    }
}

void movegen_generate_sliding_moves(Game *game, int piece_type, MoveList *moves, int only_captures)
{
    const int color = game->turn;
    const Bitboard sliders = game->board[color][piece_type];
    const Bitboard occupancy = game->occupancy[BOTH];
    const Bitboard friendly = game->occupancy[color];
    const Bitboard opp_occ = game->occupancy[!color];

    Bitboard s = sliders;

    while (s)
    {
        int from = pop_lsb(&s);

        Bitboard attacks = 0ULL;
        switch (piece_type)
        {
            case BISHOP:
                attacks = magic_get_bishop_attacks(from, occupancy);
                break;
            case ROOK:
                attacks = magic_get_rook_attacks(from, occupancy);
                break;
            case QUEEN:
                attacks = magic_get_bishop_attacks(from, occupancy)
                        | magic_get_rook_attacks(from, occupancy);
                break;
            case KING:
                attacks = game->attack_tables_pc[KING][from];
                break;
            default:
                continue; // Not a sliding piece
        }

        // Remove own pieces
        attacks &= ~friendly;

        if (only_captures) {
            // Captures only → keep only opponent pieces
            Bitboard caps = attacks & opp_occ;
            while (caps) {
                int to = pop_lsb(&caps);
                movegen_add_move(game, MOVE(from, to, 0, 1, 0, 0, 0, 0, 0), moves);
            }
        } else {
            while (attacks) {
                int to = pop_lsb(&attacks);
                int is_capture = (opp_occ >> to) & 1ULL;
                movegen_add_move(game, MOVE(from, to, 0, is_capture, 0, 0, 0, 0, 0), moves);
            }
        }
    }
}

static inline int movegen_can_castle_through_bb(Game *game, int sq, Bitboard occupied, Bitboard enemy_attacks)
{
    // Square must be empty and not attacked
    return !((occupied >> sq) & 1ULL) && !((enemy_attacks >> sq) & 1ULL);
}

void movegen_generate_castle_moves(Game *game, MoveList *moves)
{
    if (!game) return;

    int color = game->turn;
    if (!board_has_castling_rights(game, color)) return;

    int king_square = board_find_king(game, color);
    Bitboard occupied = game->occupancy[BOTH];
    Bitboard enemy_attacks = game->attack_map_full[!color];

    // If king is in check, no castling
    if ((enemy_attacks >> king_square) & 1ULL) return;

    // Kingside
    int kingside_right = (color == WHITE) ? WHITE_KINGSIDE : BLACK_KINGSIDE;
    if (board_has_castling_rights_side(game, kingside_right)) {
        int f_sq = king_square + 1;
        int g_sq = king_square + 2;

        if (movegen_can_castle_through_bb(game, f_sq, occupied, enemy_attacks) &&
            movegen_can_castle_through_bb(game, g_sq, occupied, enemy_attacks))
        {
            movegen_add_move(game, MOVE(king_square, g_sq, 0, 0, 0, 0, 0, 0, 1), moves);
        }
    }

    // Queenside
    int queenside_right = (color == WHITE) ? WHITE_QUEENSIDE : BLACK_QUEENSIDE;
    if (board_has_castling_rights_side(game, queenside_right)) {
        int d_sq = king_square - 1;
        int c_sq = king_square - 2;
        int b_sq = king_square - 3;

        // b_sq only needs to be empty, no attack requirement
        if (movegen_can_castle_through_bb(game, d_sq, occupied, enemy_attacks) &&
            movegen_can_castle_through_bb(game, c_sq, occupied, enemy_attacks) &&
            !((occupied >> b_sq) & 1ULL))
        {
            movegen_add_move(game, MOVE(king_square, c_sq, 0, 0, 0, 0, 0, 0, 1), moves);
        }
    }
}

void movegen_generate_pseudo_moves(Game *game, MoveList *moves, int only_captures)
{
    if (!game) return;

    moves->count = 0;

    // Generate pawn moves
    movegen_generate_pawn_moves(game, moves, only_captures);
    
    // Generate knight moves
    movegen_generate_knight_moves(game, moves, only_captures);

    // Generate sliding moves (bishop, rook, queen and king)
    movegen_generate_sliding_moves(game, BISHOP, moves, only_captures);
    movegen_generate_sliding_moves(game, ROOK, moves, only_captures);
    movegen_generate_sliding_moves(game, QUEEN, moves, only_captures);
    movegen_generate_sliding_moves(game, KING, moves, only_captures);

    // Generate castle moves
    if (only_captures == 0) movegen_generate_castle_moves(game, moves);
}

void movegen_generate_legal_moves(Game *game, MoveList *moves, int only_captures)
{
    if (!game) return;

    moves->count = 0;

    movegen_generate_pseudo_moves(game, moves, only_captures);

    int legal_move_count = 0;

    for (int i = 0; i < moves->count; i++)
    {
        Move move = moves->moves[i];

        board_make_move(game, move, MAKE_MOVE_LIGHT);

        if (!board_is_king_in_check(game, !game->turn))
        {
            moves->moves[legal_move_count] = move;
            legal_move_count++;
        }
        
        board_unmake_move(game, MAKE_MOVE_LIGHT);

        continue;
    }

    moves->count = legal_move_count;
}
