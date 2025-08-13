#include <search/eval.h>

#include <board/board.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

const int piece_values[] = {0, 100, 300, 350, 500, 900};

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

static inline double eval_calculate_endgame_weight(Game *game)
{
    if (!game) return 0.0;

    int piece_count = 0;

    for (int color = WHITE; color <= BLACK; color++)
    {
        Bitboard occupancy = game->occupancy[color];

        while (occupancy)
        {
            occupancy &= occupancy - 1;
            piece_count++;
        }
    }

    // Sigmoid function centered around 12 pieces, steeper curve with factor 0.5
    return 1.0 / (1.0 + exp(0.5 * (piece_count - 12)));
}

int eval_check_bishop_pairs(Game *game)
{
    if (!game) return 0;

    int score = 0;

    Bitboard bishops[2] = {game->board[WHITE][BISHOP], game->board[BLACK][BISHOP]};

    for (int color = WHITE; color <= BLACK; color++)
    {
        Bitboard board = bishops[color];

        int count = __builtin_popcountll(board);
        int perspective = (color == WHITE) ? 1 : -1;

        if (count == 2) score += BISHOP_PAIR_BONUS * perspective;
    }

    return score;
}

int eval_center_control(Game *game)
{
    if (!game) return 0;

    int score = 0;
   
    const int center_squares[4] = {27, 28, 35, 36}; /* d4, e4, d5, e5 */

    for (int i = 0; i < 4; i++)
    {
        int sq = center_squares[i];

        Piece piece = board_get_square(game, sq);
        
        int type = GET_TYPE(piece);
        int color = GET_COLOR(piece);
        int perspective = (color == WHITE) ? 1 : -1;

        // Bonus for occupying the center with a pawn
        if (type == PAWN)
        {
            score += PAWN_CENTER_CONTROL_BONUS * perspective;
        }

        // Bonus for controlling center square via attack
        if (game->attack_map_full[WHITE] & (1ULL << sq))
            score += CENTER_CONTROL_ATTACK_BONUS;

        if (game->attack_map_full[BLACK] & (1ULL << sq))
            score -= CENTER_CONTROL_ATTACK_BONUS;
    }

    return score;
}

int eval_material(Game *game)
{
    if (!game) return 0;

    int piece_count[2][7] = {{0}};

    int score = 0;

    for (int color = WHITE; color <= BLACK; color++)
    {
        for (int piece_type = PAWN; piece_type <= QUEEN; piece_type++)
        {
            Bitboard pieces = game->board[color][piece_type];

            while (pieces)
            {
                piece_count[color][piece_type]++;

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
        for (int piece = PAWN; piece <= KING; piece++)
        {
            Bitboard pieces = game->board[color][piece];

            while (pieces)
            {
                int square = __builtin_ctzll(pieces);
                int real_square = (color == BLACK) ? square : mirror[square];
                int perspective = (color == WHITE) ? 1 : -1;

                if (piece == KING)
                {
                    double endgame_weight = eval_calculate_endgame_weight(game); // Should be float between 0 and 1
                    
                    score += (int)((1.0f - endgame_weight) * eval_king_mid_pst[real_square]
                                  + endgame_weight * eval_king_end_pst[real_square]) * perspective;
                }
                else
                {
                    score += eval_pst_table[piece - 1][real_square] * perspective;
                }

                pieces &= pieces - 1;
            }
        }
    }

    return score;
}

int eval_position(Game *game)
{
    int score = 0;

    score += eval_material(game);
    score += eval_piece_squares(game);
    score += eval_center_control(game);
    score += eval_check_bishop_pairs(game);

    return (game->turn == WHITE) ? score : -score;
}