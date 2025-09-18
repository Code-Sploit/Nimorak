#include <core/rune.hpp>
#include <utils/config.hpp>
#include <core/eval.hpp>

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sstream>

namespace Rune {
    class Game;
}

namespace Config {
    using OptionSetter = void (*)(Rune::Game& game, const char* value);

    struct OptionHandler {
        const char* name;
        OptionSetter setter;
    };

    // --- Setters for movegen ---
    void set_movegen_legal(Rune::Game& game, const char* value) {
        game.config.moveGen.doLegalMoveFiltering = std::atoi(value) != 0;
    }
    void set_movegen_captures(Rune::Game& game, const char* value) {
        game.config.moveGen.doOnlyCaptures = std::atoi(value) != 0;
    }

    // --- Setters for eval ---
    void set_eval_material(Rune::Game& game, const char* value) {
        game.config.eval.doMaterial = std::atoi(value) != 0;
    }
    void set_eval_piece_squares(Rune::Game& game, const char* value) {
        game.config.eval.doPieceSquares = std::atoi(value) != 0;
    }
    void set_eval_mobility(Rune::Game& game, const char* value) {
        game.config.eval.doMobility = std::atoi(value) != 0;
    }
    void set_eval_bishop_pair(Rune::Game& game, const char* value) {
        game.config.eval.doBishopPair = std::atoi(value) != 0;
    }
    void set_eval_pawn_structure(Rune::Game& game, const char* value) {
        game.config.eval.doPawnStructure = std::atoi(value) != 0;
    }
    void set_eval_king_safety(Rune::Game& game, const char* value) {
        game.config.eval.doKingSafety = std::atoi(value) != 0;
    }

    // --- Setters for search ---
    void set_search_quiescense(Rune::Game& game, const char* value) {
        game.config.search.doQuiescense = std::atoi(value) != 0;
    }
    void set_search_transpositions(Rune::Game& game, const char* value) {
        game.config.search.doTranspositions = std::atoi(value) != 0;
    }
    void set_search_beta_cutoff_history(Rune::Game& game, const char* value) {
        game.config.search.doBetaCutoffHistory = std::atoi(value) != 0;
    }
    void set_search_info(Rune::Game& game, const char* value) {
        game.config.search.doInfo = std::atoi(value) != 0;
    }
    void set_search_initial_depth(Rune::Game& game, const char* value) {
        game.config.search.initialDepth = std::atoi(value);
    }
    void set_search_maximum_depth(Rune::Game& game, const char* value) {
        game.config.search.maximumDepth = std::atoi(value);
    }
    void set_search_quiescense_depth(Rune::Game& game, const char* value) {
        game.config.search.maximumQuiescenseDepth = std::atoi(value);
    }
    void set_search_opening_book(Rune::Game& game, const char* value) {
        game.config.search.doOpeningBook = std::atoi(value) != 0;
    }

    // --- Option registry ---
    OptionHandler option_table[] = {
        // movegen
        { "movegenDoLegalMoveFiltering", set_movegen_legal },
        { "movegenDoOnlyCaptures",        set_movegen_captures },

        // eval
        { "evalDoMaterial",       set_eval_material },
        { "evalDoPieceSquares",  set_eval_piece_squares },
        { "evalDoMobility",       set_eval_mobility },
        { "evalDoBishopPair",    set_eval_bishop_pair },
        { "evalDoPawnStructure", set_eval_pawn_structure },
        { "evalDoKingSafety",    set_eval_king_safety },

        // search
        { "searchDoQuiescense",           set_search_quiescense },
        { "searchDoTranspositions",       set_search_transpositions },
        { "searchDoBetaCutoffHistory",  set_search_beta_cutoff_history },
        { "searchDoInfo",                 set_search_info },
        { "searchInitialDepth",           set_search_initial_depth },
        { "searchMaximumDepth",           set_search_maximum_depth },
        { "searchMaximumQuiescenseDepth",set_search_quiescense_depth },
        { "searchDoOpeningBook",         set_search_opening_book },

        { nullptr, nullptr }
    };

    // --- Main dispatcher ---
    int setOption(Rune::Game& game, const std::string& name, const std::string& value) {
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
    void handleInput(Rune::Game& game, const std::string& input) {
        std::istringstream iss(input);
        std::string command, name, value;

        if (iss >> command >> name >> value) {
            if (command == "setoption") {
                setOption(game, name, value);
            }
        }
    }
} // namespace Config