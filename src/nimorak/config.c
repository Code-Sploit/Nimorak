#include <nimorak.h>
#include <nimorak/config.h>
#include <search/eval.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// --- Option handling system ---

typedef void (*OptionSetter)(Game *game, const char *value);

typedef struct {
    const char *name;
    OptionSetter setter;
} OptionHandler;

// --- Setters for movegen ---
void set_movegen_legal(Game *game, const char *value) {
    game->config->movegen.do_legal_move_filtering = atoi(value) != 0;
}
void set_movegen_captures(Game *game, const char *value) {
    game->config->movegen.do_only_captures = atoi(value) != 0;
}

// --- Setters for eval ---
void set_eval_material(Game *game, const char *value) {
    game->config->eval.do_material = atoi(value) != 0;

    eval_reinit(game);
}
void set_eval_piece_squares(Game *game, const char *value) {
    game->config->eval.do_piece_squares = atoi(value) != 0;

    eval_reinit(game);
}
void set_eval_center(Game *game, const char *value) {
    game->config->eval.do_center_control = atoi(value) != 0;
    
    eval_reinit(game);
}
void set_eval_king_safety(Game *game, const char *value) {
    game->config->eval.do_king_safety = atoi(value) != 0;

    eval_reinit(game);
}
void set_eval_pawn_structure(Game *game, const char *value) {
    game->config->eval.do_pawn_structure = atoi(value) != 0;

    eval_reinit(game);
}

// --- Setters for search ---
void set_search_quiescense(Game *game, const char *value) {
    game->config->search.do_quiescense = atoi(value) != 0;
}
void set_search_transpositions(Game *game, const char *value) {
    game->config->search.do_transpositions = atoi(value) != 0;
}
void set_search_killer_moves(Game *game, const char *value) {
    game->config->search.do_killer_moves = atoi(value) != 0;
}
void set_search_heuristics(Game *game, const char *value) {
    game->config->search.do_heuristics = atoi(value) != 0;
}
void set_search_info(Game *game, const char *value) {
    game->config->search.do_info = atoi(value) != 0;
}
void set_search_initial_depth(Game *game, const char *value) {
    game->config->search.initial_depth = atoi(value);
}
void set_search_maximum_depth(Game *game, const char *value) {
    game->config->search.maximum_depth = atoi(value);
}
void set_search_quiescense_depth(Game *game, const char *value) {
    game->config->search.maximum_quiescense_depth = atoi(value);
}

// --- Option registry ---
OptionHandler option_table[] = {
    // movegen
    { "movegen_do_legal_move_filtering", set_movegen_legal },
    { "movegen_do_only_captures",        set_movegen_captures },

    // eval
    { "eval_do_material",        set_eval_material },
    { "eval_do_piece_squares",   set_eval_piece_squares },
    { "eval_do_center_control",  set_eval_center },
    { "eval_do_king_safety",     set_eval_king_safety },

    // search
    { "search_do_quiescense",           set_search_quiescense },
    { "search_do_transpositions",       set_search_transpositions },
    { "search_do_killer_moves",         set_search_killer_moves },
    { "search_do_heuristics",           set_search_heuristics },
    { "search_do_info",                 set_search_info },

    { "search_initial_depth",           set_search_initial_depth },
    { "search_maximum_depth",           set_search_maximum_depth },
    { "search_maximum_quiescense_depth",set_search_quiescense_depth },

    { NULL, NULL }
};

// --- Main dispatcher ---
int config_set_option(Game *game, const char *name, const char *value) {
    for (int i = 0; option_table[i].name; i++) {
        if (strcmp(option_table[i].name, name) == 0) {
            option_table[i].setter(game, value);
            printf("info string set %s = %s\n", name, value);
            return 0;
        }
    }

    return 1;
}

// --- Example of parsing full input like "setoption search_initial_depth 5" ---
void config_handle_input(Game *game, const char *input) {
    char command[64], name[64], value[64];
    if (sscanf(input, "%63s %63s %63s", command, name, value) == 3) {
        if (strcmp(command, "setoption") == 0) {
            config_set_option(game, name, value);
        }
    }
}