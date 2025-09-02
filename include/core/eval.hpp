#pragma once

#include <utils/module.hpp>

namespace Nimorak {
    class Game; // forward declaration
}

namespace Evaluation {

    class Worker {
    private:
        Module::ModuleList *moduleList;  // Optional modules for evaluation

    public:
        int eval;                // Last evaluation score

        // Evaluate the current position
        int evaluate(Nimorak::Game& game);

        // Lifecycle management
        void init(Nimorak::Game& game);
        void quit(Nimorak::Game& game);
        void reinit(Nimorak::Game& game);
    };

} // namespace Evaluation