#include <core/eval.hpp>
#include <core/board.hpp>
#include <core/nimorak.hpp>

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <iostream>

namespace Nimorak {
    class Game;
}

const int pieceValues[] = {0, 100, 320, 335, 500, 900};

static const Bitboard FILE_MASKS[8] = {
    0x0101010101010101ULL,
    0x0202020202020202ULL,
    0x0404040404040404ULL,
    0x0808080808080808ULL,
    0x1010101010101010ULL,
    0x2020202020202020ULL,
    0x4040404040404040ULL,
    0x8080808080808080ULL
};

int mirror[64] = {
    56,57,58,59,60,61,62,63,
    48,49,50,51,52,53,54,55,
    40,41,42,43,44,45,46,47,
    32,33,34,35,36,37,38,39,
    24,25,26,27,28,29,30,31,
    16,17,18,19,20,21,22,23,
     8, 9,10,11,12,13,14,15,
     0, 1, 2, 3, 4, 5, 6, 7
};

void evalModuleMaterial(void* arg)
{
    Nimorak::Game& game = *static_cast<Nimorak::Game*>(arg);

    int moduleEval = 0;

    Bitboard occupancy = game.occupancy[BOTH];

    while (occupancy)
    {
        int square = Helpers::pop_lsb(occupancy);

        Piece piece = game.boardGhost[square];

        int perspective = (Helpers::get_color(piece) == WHITE) ? 1 : -1;

        moduleEval += pieceValues[Helpers::get_type(piece) - 1] * perspective;
    }

    game.eval += moduleEval;
}

namespace Evaluation {
    // --- Eval entry point ---
    int Worker::evaluate(Nimorak::Game& game)
    {
        game.eval = 0;
        game.evalModuleList.run();
        return (game.turn == WHITE) ? game.eval : -game.eval;
    }

    // --- Eval init / quit / reinit ---
    void Worker::init(Nimorak::Game& game)
    {
        int size = 0;
        if (game.config.eval.doMaterial)         size++;

        game.evalModuleList.init(size);

        if (game.config.eval.doMaterial)         game.evalModuleList.add(evalModuleMaterial, &game, "evalModuleMaterial");
    }

    void Worker::quit(Nimorak::Game& game)
    {
        game.evalModuleList.free();
    }

    void Worker::reinit(Nimorak::Game& game)
    {
        quit(game);
        init(game);
    }
}