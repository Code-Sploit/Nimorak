#pragma once

#include <string>
#include <core/nimorak.hpp>  // assumes Game, Move, ZobristHash, etc. are defined here
#include <core/board.hpp>

namespace Nimorak {
    class Game; // forward declaration
}

namespace Zobrist {

    constexpr int NUM_PIECE_TYPES = 12;
    constexpr int NUM_SQUARES     = 64;
    constexpr int NUM_CASTLING    = 16;
    constexpr int NUM_ENPASSANT   = 8;

    // Compute the Zobrist hash of the current position
    ZobristHash compute(Nimorak::Game& game);

    // Update hash after a board change
    void updateBoard(Nimorak::Game& game);

    // Update hash after a move
    void updateMove(Nimorak::Game& game, Move move, Nimorak::State& oldState);

    // Initialize Zobrist random keys
    void init();

    // Convert a hash to a string representation (for debugging)
    std::string hashToString(ZobristHash hash);

} // namespace Zobrist