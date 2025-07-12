#include "nimorak.h"
#include "constants.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

GameState *game_new(int turn)
{
    GameState *game = calloc(1, sizeof(GameState));

    if (!game) return NULL;

    game->turn = turn;

    game->can_white_castle_queenside = false;
    game->can_white_castle_kingside  = false;
    game->can_black_castle_queenside = false;
    game->can_black_castle_kingside  = false;

    game->en_passant_square = -1;

    game->move_count = 0;

    game->new_game_starting = true;

    game->permalock_black_castle = false;
    game->permalock_white_castle = false;

    return game;
}

void game_del(GameState *game)
{
    if (game) free(game);
}

void game_clone(GameState *dest, const GameState *src)
{
    if (!dest || !src) return;
    memcpy(dest, src, sizeof(GameState));
}

void nimorak_startup()
{
    printf("Nimorak V1.0 by Samuel 't Hart\n");
}