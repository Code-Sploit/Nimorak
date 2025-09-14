#include <core/nimorak.hpp>
#include <utils/config.hpp>
#include <core/eval.hpp>

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sstream>

namespace Nimorak {
    class Game;
}

namespace Config {
    using OptionSetter = void (*)(Nimorak::Game& game, const char* value);

    struct OptionHandler {
        const char* name;
        OptionSetter setter;
    };

    // --- Setters for movegen ---
    void set_movegen_legal(Nimorak::Game& game, const char* value) {
        game.config.moveGen.doLegalMoveFiltering = std::atoi(value) != 0;
    }
    void set_movegen_captures(Nimorak::Game& game, const char* value) {
        game.config.moveGen.doOnlyCaptures = std::atoi(value) != 0;
    }

    // --- Setters for eval ---
    void set_eval_material(Nimorak::Game& game, const char* value) {
        game.config.eval.doMaterial = std::atoi(value) != 0;
    }
    void set_eval_piece_squares(Nimorak::Game& game, const char* value) {
        game.config.eval.doPieceSquares = std::atoi(value) != 0;
    }
    void set_eval_mobility(Nimorak::Game& game, const char* value) {
        game.config.eval.doMobility = std::atoi(value) != 0;
    }
    void set_eval_bishop_pair(Nimorak::Game& game, const char* value) {
        game.config.eval.doBishopPair = std::atoi(value) != 0;
    }
    void set_eval_pawn_structure(Nimorak::Game& game, const char* value) {
        game.config.eval.doPawnStructure = std::atoi(value) != 0;
    }

    // --- Setters for search ---
    void set_search_quiescense(Nimorak::Game& game, const char* value) {
        game.config.search.doQuiescense = std::atoi(value) != 0;
    }
    void set_search_transpositions(Nimorak::Game& game, const char* value) {
        game.config.search.doTranspositions = std::atoi(value) != 0;
    }
    void set_search_beta_cutoff_history(Nimorak::Game& game, const char* value) {
        game.config.search.doBetaCutoffHistory = std::atoi(value) != 0;
    }
    void set_search_info(Nimorak::Game& game, const char* value) {
        game.config.search.doInfo = std::atoi(value) != 0;
    }
    void set_search_initial_depth(Nimorak::Game& game, const char* value) {
        game.config.search.initialDepth = std::atoi(value);
    }
    void set_search_maximum_depth(Nimorak::Game& game, const char* value) {
        game.config.search.maximumDepth = std::atoi(value);
    }
    void set_search_quiescense_depth(Nimorak::Game& game, const char* value) {
        game.config.search.maximumQuiescenseDepth = std::atoi(value);
    }

    // --- Option registry ---
    OptionHandler option_table[] = {
        // movegen
        { "movegen_do_legal_move_filtering", set_movegen_legal },
        { "movegen_do_only_captures",        set_movegen_captures },

        // eval
        { "eval_do_material",       set_eval_material },
        { "eval_do_piece_squares",  set_eval_piece_squares },
        { "eval_do_mobility",       set_eval_mobility },
        { "eval_do_bishop_pair",    set_eval_bishop_pair },
        { "eval_do_pawn_structure", set_eval_pawn_structure },

        // search
        { "search_do_quiescense",           set_search_quiescense },
        { "search_do_transpositions",       set_search_transpositions },
        { "search_do_beta_cutoff_history",  set_search_beta_cutoff_history },
        { "search_do_info",                 set_search_info },
        { "search_initial_depth",           set_search_initial_depth },
        { "search_maximum_depth",           set_search_maximum_depth },
        { "search_maximum_quiescense_depth",set_search_quiescense_depth },

        { nullptr, nullptr }
    };

    // --- Main dispatcher ---
    int setOption(Nimorak::Game& game, const std::string& name, const std::string& value) {
        for (int i = 0; option_table[i].name; i++) {
            if (name == option_table[i].name) {  // no more strcmp
                option_table[i].setter(game, value.c_str());  // still need c_str for your setter funcs
                std::cout << "info string set " << name << " = " << value << "\n";
                return 0;
            }
        }
        return 1;
    }

    // --- Example input parser ---
    void handleInput(Nimorak::Game& game, const std::string& input) {
        std::istringstream iss(input);
        std::string command, name, value;

        if (iss >> command >> name >> value) {
            if (command == "setoption") {
                setOption(game, name, value);
            }
        }
    }
} // namespace Config