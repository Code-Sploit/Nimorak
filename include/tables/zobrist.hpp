#pragma once

#include <string>
#include <core/rune.hpp>  // assumes Game, Move, ZobristHash, etc. are defined here
#include <core/board.hpp>

namespace Rune {
    class Game; // forward declaration
}

namespace Zobrist {

    constexpr int NUM_PIECE_TYPES = 12;
    constexpr int NUM_SQUARES     = 64;
    constexpr int NUM_CASTLING    = 16;
    constexpr int NUM_ENPASSANT   = 8;

    // Compute the Zobrist hash of the current position
    ZobristHash compute(Rune::Game& game);

    // Update hash after a board change
    void updateBoard(Rune::Game& game);

    // Update hash after a move
    void updateMove(Rune::Game& game, Move move, Rune::State& oldState);

    // Initialize Zobrist random keys
    void init();

    // Convert a hash to a string representation (for debugging)
    std::string hashToString(ZobristHash hash);

} // namespace Zobrist