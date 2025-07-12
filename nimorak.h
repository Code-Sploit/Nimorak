#ifndef NIMORAK_H
#define NIMORAK_H

#include <stdint.h>
#include "constants.h"

typedef struct
{
    int from;
    int to;

    int promotion;
    int capture;

    int piece;

    int promotion_piece;

    int is_castle_queen_side;
    int is_castle_king_side;

    int is_en_passant;

    float eval;
} Move;

typedef struct
{
    Move movelist[MAX_LEGAL_MOVES];

    int move_count;

    int board[64];

    int turn;

    int can_white_castle_queenside;
    int can_black_castle_queenside;
    int can_white_castle_kingside;
    int can_black_castle_kingside;

    int has_white_castled;
    int has_black_castled;

    int en_passant_square;

    int attack_table_white[64];
    int attack_table_black[64];

    int permalock_white_castle;
    int permalock_black_castle;

    int new_game_starting;
} GameState;

GameState *game_new();

void game_del(GameState *game);

void game_clone(GameState *dest, const GameState *src);

void nimorak_startup();

#endif