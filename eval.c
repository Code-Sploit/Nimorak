#include "eval.h"
#include "board.h"

#include <stdio.h>

const int piece_values[] = {0, 100, 300, 350, 500, 900};

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

static inline const char *square_to_text(int square) {
    static char square_str[3];
    int file = square % 8;
    int rank = square / 8;

    square_str[0] = 'a' + file;
    square_str[1] = '1' + rank;
    square_str[2] = '\0';

    return square_str;
}

int eval_position(Game *game)
{
    if (!game) return 0;

    //printf("Evaluating position...\n\n");

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

  //  printf("\nFinal evaluation: %d\n", score);

    return score;
}