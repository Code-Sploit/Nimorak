#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>

#include <nimorak.h>

static inline bool IS_SLIDING_PIECE(int piece_type)
{
    return (piece_type == BISHOP || piece_type == ROOK || piece_type == QUEEN);
}

inline void board_set_square(Game *game, int sq, Piece piece)
{
    const uint64_t bit = 1ULL << sq;

    Piece old_piece = game->board_ghost[sq];
    int old_type = GET_TYPE(old_piece);

    // Remove old piece
    if (old_type != EMPTY) {
        int old_color = GET_COLOR(old_piece);
        game->board[old_color][old_type] &= ~bit;
        game->occupancy[old_color] &= ~bit;
    }

    // Place new piece
    int new_type = GET_TYPE(piece);
    if (new_type != EMPTY) {
        int new_color = GET_COLOR(piece);
        game->board[new_color][new_type] |= bit;
        game->occupancy[new_color] |= bit;
    }

    game->board_ghost[sq] = piece;

    // Combined occupancy
    game->occupancy[BOTH] = game->occupancy[WHITE] | game->occupancy[BLACK];
}

char* board_generate_fen(Game *game);
char *board_get_checkers(Game *game);

void board_load_fen(Game *game, const char *fen_string);

void board_print(Game *game);
void board_make_move(Game *game, Move move, int generation_type);
void board_unmake_move(Game *game, int generation_type);
void board_make_null_move(Game *game);
void board_unmake_null_move(Game *game);

int board_is_on_rank(int square, int rank);
int board_find_king(const Game *game, int color);

inline int board_is_king_in_check(const Game *game, int color)
{
    int king_square = board_find_king(game, color);
    if ((unsigned)king_square >= 64) return 0;  // safer cast & check

    AttackTable attacks = game->attack_map_full[!color];
    return (attacks & (1ULL << king_square)) != 0;
}

const char *board_square_to_name(int square);

const char *board_move_to_string(Move move);

Move board_parse_move(Game *game, const char *move_str);

int board_has_castling_rights(Game *game, int color);

int board_has_castling_rights_side(Game *game, int side);

inline Piece board_get_square(const Game *game, int square)
{
    // Optionally keep safety check if needed:
    // if (!game || (unsigned)square >= 64) return MAKE_PIECE(EMPTY, EMPTY);
    return game->board_ghost[square];
}

bool board_is_same_line(int from, int to, int offset);

bool board_move_gives_check(Game *game, Move move);

bool board_is_same_ray(int square_a, int square_b);

Bitboard board_get_sliding_pieces_bitboard(Game *game, int color);

#endif
