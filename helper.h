#ifndef HELPER_H
#define HELPER_H

#include "nimorak.h"

char *translate_square_to_string(int square);

int piece_from_char(char c);

int is_color(int color, int turn);

int is_enemy_piece(int color, int turn);

int square_from_string(const char *sq_str);

int file_to_index(char file);

int rank_to_index(char rank);

void parse_move_str(const char *move_str, int *from_sq, int *to_sq);

int find_king(GameState *game, int color);

int is_king_in_check(GameState *game, int color);

int get_square(GameState *game, int square);

int is_square_attacked_by(GameState *game, int square, int color);

const char* piece_to_char(int piece);

int can_king_castle(GameState *game, int color, int side);

int square_from_coords(const char* coords);

char get_promotion_piece(Move move);

int piece_value(int piece);

void print_board(GameState *game);

void clear_board(GameState *game);

#endif