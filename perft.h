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

typedef struct {
    int depth;
    uint64_t expected;
} DepthExpectation;

typedef struct {
    const char *fen;
    DepthExpectation expectations[5];  // Adjust number depending on how many depths you want per position
    int num_expectations;
} PerftTest;

long long perft(GameState *game, int depth);

uint64_t perft_divide(GameState *game, int depth, int debug);

void perft_debug(GameState *game, int depth, const char *root_move_str);

PerftResult perft_detailed(GameState *game, int depth);

void perft_print_breakdown(GameState *game, int depth);

void perft_run_test_positions(GameState *game);

#endif