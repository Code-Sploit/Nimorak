#include "eval.h"
#include "board.h"

#include <stdio.h>

const int piece_values[] = {0, 100, 300, 350, 500, 900};

int eval_pst_table[5][64] = {
    {0,  0,  0,   0,   0,  0,  0,  0,
    60, 60, 60,  60,  60, 60, 60, 60,
    20, 30, 40,  60,  60, 40, 30, 20,
    10, 20, 40, 100, 100, 40, 20, 10,
     0, 10, 30,  90,  90, 30, 10,  0,
    10,  0,-10,  20,  20,-10,  0, 10,
    10, 20, 20, -30, -30, 20, 20, 10,
     0,  0,  0,   0,   0,  0,  0,  0},

    {-50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50},

    {-20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20},

    {0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0},

    {-20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -10,  5,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20}
};

int pst_king_midgame[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

int pst_king_endgame[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

static inline const char *color_to_text(int color)
{
    switch (color)
    {
        case WHITE: return "White";
        case BLACK: return "Black";
    }

    return "-";
}

static inline const char *piece_to_text(int piece_type)
{
    switch (piece_type)
    {
        case PAWN: return "Pawn";
        case KNIGHT: return "Knight";
        case BISHOP: return "Bishop";
        case ROOK: return "Rook";
        case QUEEN: return "Queen";
    }

    return "-";
}

static inline int invert_piece_table(int square)
{
    int rank = square / 8;
    int file = square % 8;
    int flipped_rank = 7 - rank;
    return flipped_rank * 8 + file;
}

static inline const char *square_to_text(int square) {
    static char square_str[3];
    int file = square % 8;
    int rank = square / 8;

    square_str[0] = 'a' + file;
    square_str[1] = '1' + rank;
    square_str[2] = '\0';

    return square_str;
}

int eval_material(Game *game)
{
    if (!game) return 0;

    int piece_count[2][7];

    int score = 0;

    for (int color = WHITE; color <= BLACK; color++)
    {
        for (int piece_type = PAWN; piece_type <= QUEEN; piece_type++)
        {
            Bitboard pieces = game->board[color][piece_type];

            while (pieces)
            {
                int square = __builtin_ctzll(pieces);

                piece_count[color][piece_type]++;

               // printf("[EVAL]: Found %s %s on square %s\n", color_to_text(color), piece_to_text(piece_type), square_to_text(square));

                score += piece_values[piece_type] * ((color == WHITE) ? 1 : -1);

                pieces &= pieces - 1;
            }
        }
    }

    return score;
}

int eval_piece_squares(Game *game)
{
    if (!game) return 0;

    int score = 0;

    for (int color = WHITE; color <= BLACK; color++)
    {
        for (int piece = PAWN; piece <= QUEEN; piece++)
        {
            Bitboard pieces = game->board[color][piece];

            while (pieces)
            {
                int square = __builtin_ctzll(pieces);
                int real_square = (color == BLACK && piece == PAWN) ? invert_piece_table(square) : square;
                int perspective = (color == WHITE) ? 1 : -1;

                score += eval_pst_table[piece - 1][real_square] * perspective;

                pieces &= pieces - 1;
            }
        }
    }

    return score;
}

int eval_position(Game *game)
{
    if (!game) return 0;

    //printf("Evaluating position...\n\n");

    int score = 0;

    score += eval_material(game);
    score += eval_piece_squares(game);

    return (game->turn == WHITE) ? score : -score;
}