#ifndef PERFT_H
#define PERFT_H

#include "board.h"

typedef struct {
    long long nodes;
    long long captures;
    long long promotions;
    long long castles;
    long long en_passants;
    long long checks;
    long long quiet_moves;
} PerftResult;

long long perft(GameState *game, int depth);

void perft_divide(GameState *game, int depth);

void perft_debug(GameState *game, int depth, const char *root_move_str);

PerftResult perft_detailed(GameState *game, int depth);

void perft_print_breakdown(GameState *game, int depth);

#endif