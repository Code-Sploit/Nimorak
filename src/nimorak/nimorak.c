#include <search/eval.h>

#include <nimorak.h>

#include <stdlib.h>
#include <stdio.h>

Game *game_new()
{
    Game *game = calloc(1, sizeof(Game));

    if (!game)
    {
        // Failed to allocate memory for game structure, exit

        exit(1);
    }

    game->movelist = calloc(1, sizeof(MoveList));

    // Set turn to white
    game->turn = WHITE;

    // Set enpassant square to -1, there is no enpassant possible
    game->enpassant_square = -1;

    game->repetition_table = calloc(1, sizeof(RepetitionTable));
    game->repetition_table->count = 0;

    // Alloc configuration
    game->config = calloc(1, sizeof(Configuration));

    // Set default configurations
    game->config->movegen.do_legal_move_filtering = true;
    game->config->movegen.do_only_captures        = false;
    
    game->config->eval.do_material                = true;
    game->config->eval.do_piece_squares           = true;
    game->config->eval.do_center_control          = true;
    game->config->eval.do_king_safety             = true;
    game->config->eval.do_pawn_structure          = true;

    game->config->search.do_quiescense            = true;
    game->config->search.do_transpositions        = true;
    game->config->search.do_info                  = true;
    game->config->search.do_killer_moves          = true;
    game->config->search.do_heuristics            = true;
    game->config->search.maximum_depth            = 32;
    game->config->search.initial_depth            = 6;
    game->config->search.maximum_quiescense_depth = 8;

    eval_init(game);

    return game;
}

void game_del(Game *game)
{
    if (game) free(game);
}

