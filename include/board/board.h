#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>

#include <nimorak.h>

void board_load_fen(Game *game, const char *fen_string);
void board_print(Game *game);
void board_make_move(Game *game, Move move);
void board_unmake_move(Game *game, Move move);

int board_is_on_rank(int square, int rank);
int board_find_king(const Game *game, int color);
int board_is_king_in_check(const Game *game, int color);

const char *board_square_to_name(int square);

const char *board_move_to_string(Move move);

Move board_parse_move(Game *game, const char *move_str);

int board_has_castling_rights(Game *game, int color);

int board_has_castling_rights_side(Game *game, int side);

void board_set_square(Game *game, int square, Piece piece);

Piece board_get_square(const Game *game, int square);

int board_check_square(const Game *game, int square);

bool board_is_same_line(int from, int to, int offset);

#endif
