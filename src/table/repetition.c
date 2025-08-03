#include <table/repetition.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void repetition_push(Game *game, ZobristHash hash)
{
    if (!game) return;

    game->repetition_table->stack[game->repetition_table->count++] = hash;
}

void repetition_pop(Game *game)
{
    game->repetition_table->count--;
}

void repetition_clear(Game *game)
{
    memset(game->repetition_table->stack, 0, sizeof(ZobristHash) * REPETITION_SIZE);

    game->repetition_table->count = 0;
}

int repetition_check_for_threefold(Game *game, ZobristHash hash)
{
    int count = 0;

    for (int i = 0; i < game->repetition_table->count; i++)
    {
        if (game->repetition_table->stack[i] == hash)
        {
            count++;

            if (count >= 3) return 1;
        }
    }

    return 0;
}