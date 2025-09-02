#include <utils/config.hpp>
#include <utils/uci.hpp>

int main()
{
    srand(1234);

    Nimorak::Game game;

    UCI::uciLoop(game);

    return 0;
}
