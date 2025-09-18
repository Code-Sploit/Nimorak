#include <utils/config.hpp>
#include <utils/uci.hpp>

#include <storage/openingbook.hpp>

int main()
{
    srand(1234);

    Rune::Game game;

    UCI::uciLoop(game);

    return 0;
}
