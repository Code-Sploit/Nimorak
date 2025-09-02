#include <storage/repetition.hpp>
#include <string.h>
#include <stdio.h>

namespace Repetition {

    // Safe push with overflow check
    void Table::push(ZobristHash hash)
    {
        if (count < REPETITION_SIZE) {
            stack[count++] = hash;
        } else {
            // Optional: print warning or handle overflow
            fprintf(stderr, "Warning: Repetition table overflow!\n");
        }
    }

    // Safe pop with underflow check
    void Table::pop()
    {
        if (count > 0) {
            --count;
        } else {
            fprintf(stderr, "Warning: Repetition table underflow!\n");
        }
    }

    // Clear the table
    void Table::clear()
    {
        memset(stack, 0, sizeof(ZobristHash) * REPETITION_SIZE);
        count = 0;
    }

    // Threefold repetition check
    bool Table::checkThreefold(ZobristHash hash)
    {
        int matches = 0;

        for (int i = 0; i < count; i++) {
            if (stack[i] == hash) {
                ++matches;
                if (matches >= 3) return true;
            }
        }

        return false;
    }

    // Optional: check last two moves only (for efficiency)
    bool Table::checkThreefoldRecent(int recentMoves)
    {
        int matches = 0;
        int start = (count - recentMoves >= 0) ? count - recentMoves : 0;

        for (int i = start; i < count; i++) {
            if (stack[i] == stack[count - 1]) {
                ++matches;
                if (matches >= 3) return true;
            }
        }

        return false;
    }

} // namespace Repetition