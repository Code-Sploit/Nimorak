#include <search/eval.h>

#include <board/board.h>

#include <nimorak/module.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

const int piece_values[] = {0, 100, 300, 350, 500, 900};

const int eval_pawn_structure_penalties[9] = {
    0,   // 0 pawns on file
    0,   // 1 pawn on file = fine
    20,  // doubled
    50,  // tripled
    90,  // quadrupled
    140, // etc.
    200,
    280,
    360
};

int eval_pst_table[5][64] = {
    // Pawn PST
    {
        0,  0,  0,   0,   0,  0,  0,  0,
        60, 60, 60,  60,  60, 60, 60, 60,
        20, 30, 40,  60,  60, 40, 30, 20,
        10, 20, 40, 120, 120, 40, 20, 10,
         0, 10, 30,  90,  90, 30, 10,  0,
        10,  0,-10,  20,  20,-10,  0, 10,
        10, 20, 20, -50, -50, 20, 20, 10,
         0,  0,  0,   0,   0,  0,  0,  0
    },
    // Knight PST
    {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    },
    // Bishop PST
    {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10,-10,-10,-10,-10,-20
    },
    // Rook PST
    {
         0,  0,  0,  5,  5,  0,  0,  0,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
         5, 10, 10, 10, 10, 10, 10,  5,
         0,  0,  0,  0,  0,  0,  0,  0
    },
    // Queen PST
    {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -10,  5,  5,  5,  5,  5,  0,-10,
         -5,  0,  5,  5,  5,  5,  0, -5,
          0,  0,  5,  5,  5,  5,  0, -5,
        -10,  0,  5,  5,  5,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    }
};

int eval_king_mid_pst[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

int eval_king_end_pst[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

int mirror[64] = {
    56,57,58,59,60,61,62,63,
    48,49,50,51,52,53,54,55,
    40,41,42,43,44,45,46,47,
    32,33,34,35,36,37,38,39,
    24,25,26,27,28,29,30,31,
    16,17,18,19,20,21,22,23,
     8, 9,10,11,12,13,14,15,
     0, 1, 2, 3, 4, 5, 6, 7
};

/* Eval modules */

void eval_module_material(void *arg)
{
    Game *game = (Game *) arg;

    int eval = 0;

    Bitboard occupancy = game->occupancy[BOTH];

    while (occupancy) {
        int square = __builtin_ctzll(occupancy);
        occupancy &= occupancy - 1;

        Piece piece = game->board_ghost[square];
        int type   = GET_TYPE(piece);
        int color  = GET_COLOR(piece);
        int perspective = (color == WHITE) ? 1 : -1;

        switch (type) {
            case PAWN:   eval += piece_values[PAWN]   * perspective; break;
            case KNIGHT: eval += piece_values[KNIGHT] * perspective; break;
            case BISHOP: eval += piece_values[BISHOP] * perspective; break;
            case ROOK:   eval += piece_values[ROOK]   * perspective; break;
            case QUEEN:  eval += piece_values[QUEEN]  * perspective; break;
            default: break;
        }
    }

    game->eval += eval;
}

void eval_module_pst(void *arg)
{
    Game *game = (Game *) arg;

    int eval = 0;

    Bitboard occupancy = game->occupancy[BOTH];

    while (occupancy) {
        int square = __builtin_ctzll(occupancy);
        occupancy &= occupancy - 1;

        Piece piece = game->board_ghost[square];
        int type   = GET_TYPE(piece);
        int color  = GET_COLOR(piece);
        int perspective = (color == WHITE) ? 1 : -1;
        int real_square = (color == WHITE) ? mirror[square] : square;

        switch (type) {
            case PAWN:   eval += eval_pst_table[PAWN][real_square]   * perspective; break;
            case KNIGHT: eval += eval_pst_table[KNIGHT][real_square] * perspective; break;
            case BISHOP: eval += eval_pst_table[BISHOP][real_square] * perspective; break;
            case ROOK:   eval += eval_pst_table[ROOK][real_square]   * perspective; break;
            case QUEEN:  eval += eval_pst_table[QUEEN][real_square]  * perspective; break;
            default: break;
        }
    }

    game->eval += eval;
}

void eval_module_center_control(void *arg)
{
    Game *game = (Game *) arg;

    int eval = 0;

    int center_squares[4] = {27, 28, 35, 36};

    for (int i = 0; i < 4; i++)
    {
        int square = center_squares[i];

        Piece piece = game->board_ghost[square];

        int perspective = (GET_COLOR(piece) == WHITE) ? 1 : -1;

        if (GET_TYPE(piece) == PAWN)
        {
            eval += EVAL_PAWN_CENTER_CONTROL_BONUS * perspective;
        }
    }

    game->eval += eval;
}

/* --- Eval entry point --- */

int eval_position(Game *game)
{
    // Reset eval each time
    game->eval = 0;

    // Run all modules
    module_run_list(&game->eval_module_list);

    // Return from perspective of side to move
    return (game->turn == WHITE) ? game->eval : -game->eval;
}

/* --- Eval initialization --- */

void eval_init(Game *game)
{
    int size = 0;

    if (game->config->eval.do_material)       size++;
    if (game->config->eval.do_piece_squares)  size++;
    if (game->config->eval.do_center_control) size++;

    module_init_list(&game->eval_module_list, size);

    if (game->config->eval.do_material)       module_add(&game->eval_module_list, eval_module_material, game, "eval_module_material");
    if (game->config->eval.do_piece_squares)  module_add(&game->eval_module_list, eval_module_pst, game, "eval_module_pst");
    if (game->config->eval.do_center_control) module_add(&game->eval_module_list, eval_module_center_control, game, "eval_module_center_control");
}

/* --- Eval deinitialization --- */

void eval_quit(Game *game)
{
    module_free_list(&game->eval_module_list);
}

/* --- Eval reinitialization --- */

void eval_reinit(Game *game)
{
    eval_quit(game);
    eval_init(game);
}