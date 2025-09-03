#pragma once

#include <tables/constants.hpp>

namespace Nimorak {
    class Game;
}

namespace Repetition {
    class Table {
        private:
            const static size_t REPETITION_SIZE = 16384;
            ZobristHash stack[REPETITION_SIZE];  // History of position hashes
            size_t start = 0;                             // Index of first valid hash
            size_t count = 0;                             // Number of stored hashes

        public:
            // Push a new position hash onto the stack
            void push(ZobristHash hash);

            // Pop the last position hash
            void pop();

            // Clear the repetition table
            void clear();

            // Check if the current hash occurred three times
            bool checkThreefold(ZobristHash hash);

            bool checkThreefoldRecent(size_t recentMoves);
    };
} // namespace Repetition