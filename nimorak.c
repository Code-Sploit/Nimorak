#include "nimorak.h"
#include "magic.h"

#include <stdlib.h>
#include <stdio.h>

Game *game_new()
{
    Game *game = calloc(1, sizeof(Game));

    if (!game)
    {
        // Failed to allocate memory for game structure, exit

        exit(1);
    }

    // Set turn to white
    game->turn = WHITE;

    // Set enpassant square to -1, there is no enpassant possible
    game->enpassant_square = -1;

    game->bishop_magics = generate_bishop_magics();
    game->rook_magics   = generate_rook_magics();

    return game;
}

void game_del(Game *game)
{
    if (game) free(game);
}
