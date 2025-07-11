#include "nimorak.h"
#include "constants.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

GameState *new_game(int turn, const char *startpos)
{
    GameState *game = calloc(1, sizeof(GameState));

    if (!game) return NULL;

    game->turn = turn;

    game->can_white_castle_queenside = true;
    game->can_white_castle_kingside  = true;
    game->can_black_castle_queenside = true;
    game->can_black_castle_kingside  = true;

    game->en_passant_square = -1;

    game->move_count = 0;

    game->new_game_starting = true;

    return game;
}

void del_game(GameState *game)
{
    if (game) free(game);
}

