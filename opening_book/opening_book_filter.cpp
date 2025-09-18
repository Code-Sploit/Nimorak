#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <fstream>

#include <storage/openingbook.hpp>

int main() {
    std::unordered_set<std::string> keepFirstPawnMoves = {"b2b3", "g2g3"};
    std::unordered_set<char> pawnFilesToRemove = {'a', 'c', 'd', 'e', 'f', 'h'};

    std::vector<std::vector<std::string>> filteredBook;

    // --- Filter the book ---
    for (const auto& game : OpeningBookData::openingBook) {
        if (game.empty()) continue;

        const std::string& firstMove = game[0];

        bool remove = false;
        if (firstMove.size() == 4 && firstMove[1] == '2' && firstMove[3] == '3') {
            char file = firstMove[0];
            if (pawnFilesToRemove.count(file) && keepFirstPawnMoves.count(firstMove) == 0) {
                remove = true;
            }
        }

        if (!remove) {
            filteredBook.push_back(game);
        }
    }

    // --- Print filtered book for verification ---
    std::cout << "Filtered Opening Book:\n";
    for (const auto& game : filteredBook) {
        for (const auto& move : game) {
            std::cout << move << " ";
        }
        std::cout << "\n";
    }

    // --- Write filtered book to a header file ---
    std::ofstream outFile("filtered_opening_book.hpp");
    if (!outFile.is_open()) {
        std::cerr << "Failed to open output file.\n";
        return 1;
    }

    outFile << "#pragma once\n\n";
    outFile << "#include <vector>\n#include <string>\n\n";
    outFile << "namespace OpeningBookData {\n";
    outFile << "    const std::vector<std::vector<std::string>> openingBook = {\n";

    for (size_t i = 0; i < filteredBook.size(); ++i) {
        outFile << "        {";
        for (size_t j = 0; j < filteredBook[i].size(); ++j) {
            outFile << "\"" << filteredBook[i][j] << "\"";
            if (j + 1 < filteredBook[i].size()) outFile << ", ";
        }
        outFile << "}";
        if (i + 1 < filteredBook.size()) outFile << ",";
        outFile << "\n";
    }

    outFile << "    };\n";
    outFile << "}\n";

    outFile.close();

    std::cout << "Filtered opening book written to filtered_opening_book.hpp\n";

    return 0;
}
