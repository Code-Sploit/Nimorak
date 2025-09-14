#pragma once

#include <cstddef>  // for size_t 
#include <string>

namespace Nimorak {
    class Game;
}

namespace Config {

    // ----------------------------
    // Move generation options
    // ----------------------------
    struct MoveGenOptions {
        bool doLegalMoveFiltering = true;
        bool doOnlyCaptures = false;
    };

    // ----------------------------
    // Evaluation options
    // ----------------------------
    struct EvalOptions {
        bool doMaterial = true;
        bool doPieceSquares = true;
        bool doMobility = true;
        bool doBishopPair = true;
        bool doPawnStructure = true;
    };

    // ----------------------------
    // Search options
    // ----------------------------
    struct SearchOptions {
        bool doQuiescense = true;
        bool doTranspositions = true;
        bool doBetaCutoffHistory = true;
        bool doInfo = true;
        bool doKillerMoves = true;

        int initialDepth = 6;
        int maximumDepth = 32;
        int maximumQuiescenseDepth = 16;
    };

    // ----------------------------
    // Master configuration
    // ----------------------------
    struct Configuration {
        MoveGenOptions moveGen;
        EvalOptions eval;
        SearchOptions search;
    };

    int setOption(Nimorak::Game& game, const std::string& name, const std::string& value);
    void handleInput(Nimorak::Game& game, const std::string& input);

} // namespace Config