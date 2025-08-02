#include "movegen.h"
#include "board.h"
#include "magic.h"
#include "attack.h"

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

void movegen_generate_pawn_moves(Game *game, MoveList *moves)
{
    if (!game) return;

    const int color = game->turn;
    const int perspective = (color == WHITE) ? 1 : -1;
    const int start_rank = (color == WHITE) ? 1 : 6;
    const int promote_rank = (color == WHITE) ? 7 : 0;
    const int capture_offsets[2] = { (color == WHITE) ? 7 : -9, (color == WHITE) ? 9 : -7 };
    const int single_push = PAWN_OFFSETS[0] * perspective;
    const int double_push = PAWN_OFFSETS[1] * perspective;

    Bitboard pawns = game->board[color][PAWN];

    while (pawns)
    {
        int from = __builtin_ctzll(pawns);
        pawns &= pawns - 1;

        int rank = from / 8;
        int file = from % 8;

        int to_single = from + single_push;

        // Single push
        if (to_single < 64 && to_single >= 0 && board_get_square(game, to_single) == EMPTY)
        {
            if (board_is_on_rank(to_single, promote_rank))
                movegen_add_promotion_moves(game, from, to_single, 0, moves);
            else
            {
                movegen_add_move(game, MOVE(from, to_single, 0, 0, 0, 0, 0, 0, 0), moves);

                // Double push
                int to_double = from + double_push;
                if (rank == start_rank && board_get_square(game, to_double) == EMPTY)
                {
                    movegen_add_move(game, MOVE(from, to_double, 0, 0, 0, 0, 0, 1, 0), moves);
                }
            }
        }

        // Captures and en passant
        for (int i = 0; i < 2; i++)
        {
            int to = from + capture_offsets[i];
            if (to < 0 || to >= 64)
                continue;

            // Prevent file wrap (wrap happens from A->H or H->A across board edges)
            if ((i == 0 && file == 0) || (i == 1 && file == 7))
                continue;

            Piece target = board_get_square(game, to);

            if (target != EMPTY && GET_COLOR(target) != color)
            {
                if (board_is_on_rank(to, promote_rank))
                    movegen_add_promotion_moves(game, from, to, 1, moves);
                else
                    movegen_add_move(game, MOVE(from, to, 0, 1, 0, 0, 0, 0, 0), moves);
            }

            if (game->enpassant_square == to)
            {
                movegen_add_move(game, MOVE(from, to, 0, 1, 0, 1, 0, 0, 0), moves);
            }
        }
    }
}

void movegen_generate_knight_moves(Game *game, MoveList *moves)
{
    if (!game) return;

    int color = game->turn;
    Bitboard knights = game->board[color][KNIGHT];
    Bitboard friendly = game->occupancy[color];

    while (knights)
    {
        int from = __builtin_ctzll(knights);
        knights &= knights - 1;

        Bitboard attacks = game->attack_tables_pc[KNIGHT][from] & ~friendly;

        while (attacks)
        {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;

            // No need to check color/type since friendly squares masked out
            movegen_add_move(game, MOVE(from, to, 0, 0, 0, 0, 0, 0, 0), moves);
        }
    }
}

void movegen_generate_sliding_moves(Game *game, int piece_type, MoveList *moves)
{
    if (!game) return;

    int color = game->turn;
    Bitboard sliders = game->board[color][piece_type];
    Bitboard occupancy = game->occupancy[BOTH];

    while (sliders)
    {
        int square = __builtin_ctzll(sliders);
        sliders &= sliders - 1;

        Bitboard attacks = 0ULL;

        switch (piece_type)
        {
            case BISHOP:
                attacks = magic_get_bishop_attacks(game, square, occupancy);
                break;
            case ROOK:
                attacks = magic_get_rook_attacks(game, square, occupancy);
                break;
            case QUEEN:
                attacks = magic_get_bishop_attacks(game, square, occupancy)
                        | magic_get_rook_attacks(game, square, occupancy);
                break;
            case KING:
                attacks = game->attack_tables_pc[KING][square];
                break;
            default:
                continue; // Skip non-sliders
        }

        Bitboard attack_targets = attacks;
        while (attack_targets)
        {
            int target = __builtin_ctzll(attack_targets);
            attack_targets &= attack_targets - 1;

            Piece target_piece = board_get_square(game, target);
            int target_color = GET_COLOR(target_piece);

            if (GET_TYPE(target_piece) == EMPTY)
            {
                movegen_add_move(game, MOVE(square, target, 0, 0, 0, 0, 0, 0, 0), moves);
            }
            else if (target_color != color)
            {
                movegen_add_move(game, MOVE(square, target, 0, 1, 0, 0, 0, 0, 0), moves);
            }
        }
    }
}


static int movegen_can_castle_through(Game *game, int sq)
{
    AttackTable enemy_attack = game->attack_map_full[!game->turn];

    if (board_get_square(game, sq) != EMPTY) return 0; // square not empty
    
    if ((enemy_attack >> sq) & 1ULL) return 0; // attacked by enemy

    return 1;
}

void movegen_generate_castle_moves(Game *game, MoveList *moves)
{
    if (!game || !board_has_castling_rights(game, game->turn)) return;

    int king_square = board_find_king(game, game->turn);
    int is_white = (game->turn == WHITE);

    // Check if king is currently in check
    if (game->attack_map_full[!game->turn] & (1ULL << king_square)) return;

    // Kingside castling
    int kingside_right = is_white ? WHITE_KINGSIDE : BLACK_KINGSIDE;
    if (board_has_castling_rights_side(game, kingside_right)) {
        int f1 = king_square + 1;
        int g1 = king_square + 2;

        if (movegen_can_castle_through(game, f1) && movegen_can_castle_through(game, g1)) {
            movegen_add_move(game, MOVE(king_square, g1, 0, 0, 0, 0, 0, 0, 1), moves);
        }
    }

    // Queenside castling
    int queenside_right = is_white ? WHITE_QUEENSIDE : BLACK_QUEENSIDE;
    if (board_has_castling_rights_side(game, queenside_right)) {
        int d1 = king_square - 1;
        int c1 = king_square - 2;
        int b1 = king_square - 3;

        if (movegen_can_castle_through(game, d1) && movegen_can_castle_through(game, c1) && (GET_TYPE(board_get_square(game, b1)) == EMPTY)) {
            movegen_add_move(game, MOVE(king_square, c1, 0, 0, 0, 0, 0, 0, 1), moves);
        }
    }
}

void movegen_generate_pseudo_moves(Game *game, MoveList *moves)
{
    if (!game) return;

    moves->count = 0;

    // Generate pawn moves
    movegen_generate_pawn_moves(game, moves);
    
    // Generate knight moves
    movegen_generate_knight_moves(game, moves);

    // Generate sliding moves (bishop, rook, queen and king)
    movegen_generate_sliding_moves(game, BISHOP, moves);
    movegen_generate_sliding_moves(game, ROOK, moves);
    movegen_generate_sliding_moves(game, QUEEN, moves);
    movegen_generate_sliding_moves(game, KING, moves);

    // Generate castle moves
    movegen_generate_castle_moves(game, moves);
}

void movegen_generate_legal_moves(Game *game, MoveList *moves)
{
    if (!game) return;

    moves->count = 0;

    movegen_generate_pseudo_moves(game, moves);

    int move_count = moves->count;
    int legal_move_count = 0;

    for (int i = 0; i < move_count; i++)
    {
        Move move = moves->moves[i];

        if (board_get_square(game, GET_FROM(move)) == 0) continue;

        board_make_move(game, move);

        if (!board_is_king_in_check(game, !game->turn))
        {
            moves->moves[legal_move_count] = move;
            legal_move_count++;
        }
        
        board_unmake_move(game, move);
    }

    moves->count = legal_move_count;
}
