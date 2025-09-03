#include <storage/repetition.hpp>
#include <iostream>
#include <string>
#include <cstring>

namespace Repetition {
    // Push a new hash; overwrites oldest if full
    void Table::push(ZobristHash hash) {
        if (count < REPETITION_SIZE) {
            stack[(start + count) % REPETITION_SIZE] = hash;
            ++count;
        } else {
            // Table is full; overwrite oldest
            stack[start] = hash;
            start = (start + 1) % REPETITION_SIZE;
        }
    }

    // Pop the last hash
    void Table::pop() {
        if (count > 0) {
            --count;
        } else {
            fprintf(stderr, "Warning: Repetition table underflow!\n");
        }
    }

    // Clear the table
    void Table::clear() {
        memset(stack, 0, sizeof(stack));
        start = 0;
        count = 0;
    }

    // Check threefold repetition
    bool Table::checkThreefold(ZobristHash hash) {
        int matches = 0;
        for (size_t i = 0; i < count; ++i) {
            size_t idx = (start + i) % REPETITION_SIZE;
            if (stack[idx] == hash) ++matches;
            if (matches >= 3) return true;
        }
        return false;
    }

    // Check last `recentMoves` hashes only
    bool Table::checkThreefoldRecent(size_t recentMoves) {
        int matches = 0;
        size_t movesToCheck = (recentMoves < count) ? recentMoves : count;

        for (size_t i = count - movesToCheck; i < count; ++i) {
            size_t idx = (start + i) % REPETITION_SIZE;
            if (stack[idx] == stack[(start + count - 1) % REPETITION_SIZE]) {
                ++matches;
                if (matches >= 3) return true;
            }
        }

        return false;
    }
} // namespace Repetition