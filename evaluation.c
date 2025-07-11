#include "evaluation.h"
#include "constants.h"
#include "helper.h"

#include <stdbool.h>

int HANGING_PENALTY = 100;
int PST_SCALE = 10;

static bool is_early_penalty_square(int square) {
    // Center squares: d4, e4, d5, e5 (indexes 27,28,35,36)
    return (square == 27 || square == 28 || square == 35 || square == 36);
}

static bool white_pawn_in_center(GameState *game) {
    // Check if white pawn occupies any of d4, e4, d5, e5 (27,28,35,36)
    return (game->board[27] == W_PAWN || game->board[28] == W_PAWN ||
            game->board[35] == W_PAWN || game->board[36] == W_PAWN);
}

static bool black_pawn_in_center(GameState *game) {
    // Check if black pawn occupies any of d4, e4, d5, e5 (27,28,35,36)
    return (game->board[27] == B_PAWN || game->board[28] == B_PAWN ||
            game->board[35] == B_PAWN || game->board[36] == B_PAWN);
}

static inline int mirror_index(int idx) {
    return (7 - idx / 8) * 8 + (idx % 8);
}

static int count_piece_moves(GameState *game, int piece)
{
    if (!game) return -1;

    int count = 0;

    for (int i = 0; i < game->move_count; i++)
    {
        if (game->movelist[i].piece == piece) count++;
    }

    return count;
}

static int king_safety(GameState *game, int color)
{
    if (!game) return 0;

    int king_square = find_king(game, color);

    int safety_score = 0;

    for (int i = 0; i < 8; i++)
    {
        int offset = KING_OFFSETS[i];
        int target = king_square + offset;

        if (get_square(game, target) == EMPTY)
        {
            safety_score -= 50;
        }
        else if (is_enemy_piece(target, game->turn))
        {
            safety_score -= 60;
        }
        else
        {
            safety_score += 50;
        }
    }

    return safety_score;
}

int evaluate_position(GameState *game)
{
    // Phase weights for game phase calculation
    const int PHASE_PAWN = 0;
    const int PHASE_KNIGHT = 1;
    const int PHASE_BISHOP = 1;
    const int PHASE_ROOK = 2;
    const int PHASE_QUEEN = 4;

    int score_white = 0;
    int score_black = 0;

    // Calculate game phase (1 = opening, 0 = endgame)
    int max_phase = (PHASE_KNIGHT + PHASE_BISHOP) * 4 + PHASE_ROOK * 4 + PHASE_QUEEN * 1; 
    // max_phase = knights + bishops (4 each) + rooks (4) + queens (1)

    int phase = max_phase;

    for (int i = 0; i < 64; i++) {
        switch(game->board[i]) {
            case W_KNIGHT:
            case B_KNIGHT:
                phase -= PHASE_KNIGHT; break;
            case W_BISHOP:
            case B_BISHOP:
                phase -= PHASE_BISHOP; break;
            case W_ROOK:
            case B_ROOK:
                phase -= PHASE_ROOK; break;
            case W_QUEEN:
            case B_QUEEN:
                phase -= PHASE_QUEEN; break;
        }
    }
   
    float phase_factor = (float)phase / max_phase; // Now 1 = endgame, 0 = opening

    bool wp_in_center = white_pawn_in_center(game);
    bool bp_in_center = black_pawn_in_center(game);

    for (int i = 0; i < 64; i++)
    {
        int piece = game->board[i];
        int file = i % 8;
        int rank = i / 8;
        int mirrored_index = (piece < 0) ? i : mirror_index(i);

        switch (piece)
        {
            case W_PAWN:
                score_white += piece_value(W_PAWN);
                score_white += pawn_pst[mirrored_index] / 10;
                if (game->black_controlled_squares[i] && !game->white_controlled_squares[i])
                    score_white -= HANGING_PENALTY + (piece_value(piece) / 5);
                break;

            case W_KNIGHT:
                score_white += piece_value(W_KNIGHT);
                score_white += knight_pst[mirrored_index] / 10;
                if (game->black_controlled_squares[i] && !game->white_controlled_squares[i])
                    score_white -= HANGING_PENALTY + (piece_value(piece) / 5);
                
                // Penalize knights occupying center squares if no white pawn there yet
                if (is_early_penalty_square(i) && !wp_in_center) {
                    score_white -= (int)(60 * phase_factor);
                }
                break;

            case W_BISHOP:
                score_white += piece_value(W_BISHOP);
                score_white += bishop_pst[mirrored_index] / 10;
                if (game->black_controlled_squares[i] && !game->white_controlled_squares[i])
                    score_white -= HANGING_PENALTY + (piece_value(piece) / 5);

                // You can add similar penalties for bishops if you want here
                break;

            case W_ROOK:
                score_white += piece_value(W_ROOK);
                score_white += rook_pst[mirrored_index] / 10;
                if (game->black_controlled_squares[i] && !game->white_controlled_squares[i])
                    score_white -= HANGING_PENALTY + (piece_value(piece) / 5);
                break;

            case W_QUEEN:
                score_white += piece_value(W_QUEEN);
                score_white += queen_pst[mirrored_index] / 10;
                if (game->black_controlled_squares[i] && !game->white_controlled_squares[i])
                    score_white -= HANGING_PENALTY + (piece_value(piece) / 5) * 5;
                break;

            case W_KING:
                score_white += piece_value(W_KING);
                score_white += king_pst[mirrored_index] / 10;
                break;

            case B_PAWN:
                score_black += piece_value(B_PAWN);
                score_black += pawn_pst[i] / 10;
                if (game->white_controlled_squares[i] && !game->black_controlled_squares[i])
                    score_black -= HANGING_PENALTY + (piece_value(piece) / 5);
                break;

            case B_KNIGHT:
                score_black += piece_value(B_KNIGHT);
                score_black += knight_pst[i] / 10;
                if (game->white_controlled_squares[i] && !game->black_controlled_squares[i])
                    score_black -= HANGING_PENALTY + (piece_value(piece) / 5);

                // Penalize black knights occupying center squares if no black pawn there yet
                if (is_early_penalty_square(i) && !bp_in_center) {
                    score_black -= (int)(80 * phase_factor);
                }
                break;

            case B_BISHOP:
                score_black += piece_value(B_BISHOP);
                score_black += bishop_pst[i] / 10;
                if (game->white_controlled_squares[i] && !game->black_controlled_squares[i])
                    score_black -= HANGING_PENALTY + (piece_value(piece) / 5);
                break;

            case B_ROOK:
                score_black += piece_value(B_ROOK);
                score_black += rook_pst[i] / 10;
                if (game->white_controlled_squares[i] && !game->black_controlled_squares[i])
                    score_black -= HANGING_PENALTY + (piece_value(piece) / 5);
                break;

            case B_QUEEN:
                score_black += piece_value(B_QUEEN);
                score_black += queen_pst[i] / 10;
                if (game->white_controlled_squares[i] && !game->black_controlled_squares[i])
                    score_black -= HANGING_PENALTY + (piece_value(piece) / 5) * 5;
                break;

            case B_KING:
                score_black += piece_value(B_KING);
                score_black += king_pst[i] / 10;
                break;
        }
    }

    if (game->can_white_castle_kingside && !game->has_white_castled) score_white += 100;
    if (game->can_white_castle_queenside && !game->has_white_castled) score_white += 100;
    if (game->can_black_castle_kingside && !game->has_black_castled) score_black += 100;
    if (game->can_black_castle_queenside && !game->has_black_castled) score_black += 100;

    score_white += king_safety(game, WHITE);
    score_black += king_safety(game, BLACK);

    score_white += 4 * count_piece_moves(game, W_BISHOP);
    score_white += 4 * count_piece_moves(game, W_KNIGHT);
    score_white += 4 * count_piece_moves(game, W_QUEEN);
    score_white += 4 * count_piece_moves(game, W_ROOK);

    score_black += 4 * count_piece_moves(game, B_BISHOP);
    score_black += 4 * count_piece_moves(game, B_KNIGHT);
    score_black += 4 * count_piece_moves(game, B_QUEEN);
    score_black += 4 * count_piece_moves(game, B_ROOK);

    return (game->turn == WHITE) ? (score_white - score_black) : (score_black - score_white);
}
