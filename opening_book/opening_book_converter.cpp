#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <storage/opening_book.hpp>

int main() {
    std::vector<std::string> converted;

    for (size_t i = 0; i < openingBookSize; i++) {
        std::istringstream iss(opening_book[i]);
        std::string move, result;
        int moveCount = 0;

        // Extract first 12 moves (12 ply = 6 full moves per side)
        while (iss >> move && moveCount < 12) {
            if (!result.empty()) result += " ";
            result += move;
            moveCount++;
        }

        converted.push_back(result);
    }

    // Print the converted array in your desired format
    std::cout << "const std::string openingBook[] = {\n";
    for (size_t i = 0; i < converted.size(); i++) {
        std::cout << "    {\"" << converted[i] << "\"}";
        if (i + 1 < converted.size()) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "};\n";

    return 0;
}