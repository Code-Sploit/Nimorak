#ifndef SEARCH_H
#define SEARCH_H

#include <nimorak.h>

#define SEARCH_INITIAL_DEPTH 5
#define INF 1000000

Move search_start(Game *game, int initial_depth);

#endif