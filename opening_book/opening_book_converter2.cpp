#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <ostream>

#include <storage/openingbook.hpp>

int main() {
    std::cout << "const std::vector<std::vector<std::string>> openingBook = {\n";

    for (size_t i = 0; i < OpeningBookData::openingBookSize; i++) {
        std::istringstream iss(OpeningBookData::openingBook[i]);
        std::string move;

        std::cout << "    {";
        bool first = true;

        while (iss >> move) {
            if (!first) std::cout << ", ";
            std::cout << "\"" << move << "\"";
            first = false;
        }

        std::cout << "}";
        if (i + 1 < OpeningBookData::openingBookSize) std::cout << ",";
        std::cout << "\n";
    }

    std::cout << "};\n";

    return 0;
}
