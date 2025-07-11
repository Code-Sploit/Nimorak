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

    int black_controlled_squares[64];
    int white_controlled_squares[64];

    int new_game_starting;
} GameState;

GameState *new_game(int turn, const char *startpos);

void del_game(GameState *game);

#endif