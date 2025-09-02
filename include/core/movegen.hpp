#pragma once

#include <core/attack.hpp>
#include <tables/constants.hpp>
#include <string>

namespace Nimorak {
    class Game; // forward declaration
}

namespace Movegen {
    class MoveList {
    private:
        int count;
        Move moves[256];

    public:
        // Constructor
        MoveList() : count(0) {
            std::fill(std::begin(moves), std::end(moves), 0);
        }

        // Get number of moves
        int size() const { return count; }
        void setsize(int size) { this->count = size; }

        // Add a move
        void add(Move move) {
            if (count < 256) {
                moves[count++] = move;
            }
        }

        // Access move by index
        Move operator[](std::size_t index) const {
            return moves[index];
        }

        Move& operator[](std::size_t index) {
            return moves[index];
        }

        // Clear move list
        void clear() { count = 0; }
    };
    
    class Worker {
    public:
        // Internal helpers
        void addPromotionMoves(MoveList& moves, int from, int to, int isCapture);
        bool canCastleThroughBitboard(int square, Bitboard occupancy, Bitboard enemyAttacks);

        // Piece move generators
        void getPawnMoves(Nimorak::Game& game, MoveList& moves, bool onlyCaptures);
        void getKnightMoves(Nimorak::Game& game, MoveList& moves, bool onlyCaptures);
        void getKingMoves(Nimorak::Game& game, MoveList& moves, bool onlyCaptures);
        void getSlidingMoves(Nimorak::Game& game, MoveList& moves, PieceType type, bool onlyCaptures);

        // Castling
        bool canCastleThrough(Nimorak::Game& game, int square, Bitboard occupancy, Bitboard enemyAttacks);
        void getCastleMoves(Nimorak::Game& game, MoveList& moves);

        // Full move generation
        void getPseudoMoves(Nimorak::Game& game, MoveList& moves, bool onlyCaptures);
        void getLegalMoves(Nimorak::Game& game, MoveList& moves, bool onlyCaptures);
    };

} // namespace Movegen