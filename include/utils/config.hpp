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
        bool doEndgame = true;
        bool doMobility = true;
        bool doKingSafety = true;
        bool doPawnStructure = true;
        bool doRookOpenFiles = true;
        bool doBishopPair = true;
        bool doKnightOutposts = true;
    };

    // ----------------------------
    // Search options
    // ----------------------------
    struct SearchOptions {
        bool doQuiescense = true;
        bool doTranspositions = true;
        bool doKillerMoves = true;
        bool doHeuristics = true;
        bool doInfo = true;

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