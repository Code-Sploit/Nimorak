#ifndef PERFT_H
#define PERFT_H

#include "nimorak.h"

long long perft(Game *game, int depth);
void perft_run_tests(Game *game);
void perft_root(Game *game, int depth);

#endif
