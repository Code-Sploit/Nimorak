#include <search/eval.h>

#include <board/board.h>

#include <stdio.h>
#include <math.h>

const int piece_values[] = {0, 100, 300, 350, 500, 900};

int eval_pst_table[5][64] = {
    {0,  0,  0,   0,   0,  0,  0,  0,
    60, 60, 60,  60,  60, 60, 60, 60,
    20, 30, 40,  60,  60, 40, 30, 20,
    10, 20, 40, 120, 120, 40, 20, 10,
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

int eval_endgame_king_to_corner(Game *game) {
    if (!game) return 0;

    double endgame_weight = eval_calculate_endgame_weight(game);
    if (endgame_weight == 0.0) return 0;

    // Example assumptions about your Game structure:
    // - int side_to_move; // side to move (0=white,1=black)
    // - int king_square[2]; // king positions for white and black
    // - int rook_count[2];  // count of rooks per side
    // - int piece_count[2]; // total pieces per side (excluding king)
    
    int attacker = game->turn;
    int defender = attacker ^ 1;

    // Get defender king position
    int ksq = board_find_king(game, defender);
    int rank = ksq / 8;
    int file = ksq % 8;

    // Corners coordinates
    int corners[4][2] = {
        {0, 0},
        {0, 7},
        {7, 0},
        {7, 7}
    };

    // Calculate min Manhattan distance to corners for defender king
    int min_dist = 14; // max manhattan dist on 8x8 is 7+7=14
    for (int i = 0; i < 4; i++) {
        int dist = abs(rank - corners[i][0]) + abs(file - corners[i][1]);
        if (dist < min_dist) min_dist = dist;
    }

    // Map distance to score: closer to corner = higher score
    // Scale to ~200 centipawns max, linearly
    int max_dist = 14;
    int base_score = 200 * (max_dist - min_dist) / max_dist;

    // Weight by endgame phase
    double weighted_score = base_score * endgame_weight;

    // Return positive score from attacker POV
    // (You may want to adjust sign depending on your eval convention)
    return (int)(weighted_score + 0.5);
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

        // Bonus for occupying the center with a pawn
        if (type == PAWN)
        {
            if (color == WHITE) score += PAWN_CENTER_CONTROL_BONUS;
            else if (color == BLACK) score -= PAWN_CENTER_CONTROL_BONUS;
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
        for (int piece = PAWN; piece <= KING; piece++)
        {
            Bitboard pieces = game->board[color][piece];

            while (pieces)
            {
                int square = __builtin_ctzll(pieces);

                if (piece == KING)
                {
                    int endgame_weight = eval_calculate_endgame_weight(game);
                    int real_square = (color == WHITE) ? square : mirror[square];

                    score += eval_king_mid_pst[real_square] + (endgame_weight * eval_king_end_pst[real_square]);
                }
                else
                {
                    int real_square = (color == WHITE) ? square : mirror[square];
                    int perspective = (color == WHITE) ? 1 : -1;

                    //if (piece == PAWN) printf("evaluating black pawn for square %d transforming to real square gives %d with eval %d\n", square, real_square, eval_pst_table[piece - 1][real_square] * perspective);

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
    if (!game) return 0;

    //printf("Evaluating position...\n\n");

    int score = 0;

    score += eval_material(game);
    score += eval_piece_squares(game);
    score += eval_endgame_king_to_corner(game);
    score += eval_center_control(game);

    return (game->turn == WHITE) ? score : -score;
}