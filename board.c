#include "nimorak.h"
#include "constants.h"
#include "board.h"
#include "attack.h"
#include "helper.h"

#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int KNIGHT_OFFSETS[] = {-17, -10, 6, 15, 17, 10, -6, -15};
int BISHOP_OFFSETS[] = {7, 9, -7, -9};
int ROOK_OFFSETS[] = {1, -1, 8, -8};
int QUEEN_OFFSETS[] = {1, -1, 7, -7, 8, -8, 9, -9};
int KING_OFFSETS[] = {1, -1, 7, -7, 8, -8, 9, -9};
int PAWN_OFFSETS[] = {8, 16, -8, -16, 7, 9, -7, -9};

int pawn_pst[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50,
     10, 10, 20, 30, 30, 20, 10, 10,
      5,  5, 10, 25, 25, 10,  5,  5,
      0,  0,  0, 20, 20,  0,  0,  0,
      5, -5,-10,  0,  0,-10, -5,  5,
      5, 10, 10,-20,-20, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0
};

int knight_pst[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

int bishop_pst[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

int rook_pst[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

int queen_pst[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

int king_pst[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

void load_fen(GameState *game, const char *fen)
{
    if (!game || !fen) return;

    int idx = 56;

    for (; *fen && *fen != ' '; fen++) {
        if (*fen == '/') {
            idx -= 16;
        } else if (isdigit(*fen)) {
            idx += *fen - '0';
        } else {
            game->board[idx++] = piece_from_char(*fen);
        }
    }

    game->turn = (*++fen == 'b') ? BLACK : WHITE;
}

void clear_move_list(GameState *game)
{
    if (!game) return;

    memset(game->movelist, 0, sizeof(game->movelist));

    game->move_count = 0;
}

void add_move(GameState *game, Move move)
{
    if (!game || game->move_count >= MAX_LEGAL_MOVES) return;
    game->movelist[game->move_count++] = move;
}

void clone_game_state(GameState *dest, const GameState *src)
{
    if (!dest || !src) return;
    memcpy(dest, src, sizeof(GameState));
}

void make_move(GameState *game, Move move)
{
    if (!game) return;

    int from = move.from, to = move.to;
    if ((unsigned)from > 63 || (unsigned)to > 63) return;

    int piece = game->board[from];

    switch (piece) {
        case W_ROOK:
            if (from == 0) game->can_white_castle_queenside = false;
            if (from == 7) game->can_white_castle_kingside = false;
            break;
        case W_KING:
            if (from == 4) {
                game->can_white_castle_kingside = false;
                game->can_white_castle_queenside = false;
            }
            break;
        case B_ROOK:
            if (from == 56) game->can_black_castle_queenside = false;
            if (from == 63) game->can_black_castle_kingside = false;
            break;
        case B_KING:
            if (from == 60) {
                game->can_black_castle_kingside = false;
                game->can_black_castle_queenside = false;
            }
            break;
    }

    if (move.piece == W_KING || move.piece == B_KING)
    {
        // White king castling
        if (move.piece == W_KING)
        {
            if (move.to - move.from == 2)
                move.is_castle_king_side = true;
            else if (move.from - move.to == 2)
                move.is_castle_queen_side = true;
        }
        // Black king castling
        else
        {
            if (move.to - move.from == 2)
                move.is_castle_king_side = true;
            else if (move.from - move.to == 2)
                move.is_castle_queen_side = true;
        }
    }

    // Revalidate castling rights
    if (game->can_white_castle_kingside)
        game->can_white_castle_kingside = can_king_castle(game, WHITE, KING_SIDE);
    if (game->can_white_castle_queenside)
        game->can_white_castle_queenside = can_king_castle(game, WHITE, QUEEN_SIDE);
    if (game->can_black_castle_kingside)
        game->can_black_castle_kingside = can_king_castle(game, BLACK, KING_SIDE);
    if (game->can_black_castle_queenside)
        game->can_black_castle_queenside = can_king_castle(game, BLACK, QUEEN_SIDE);

    if (piece == W_PAWN || piece == B_PAWN)
    {
        if (abs(move.from - move.to) == 16)
        {
            // Set en passant square to the square behind the pawn after the double step
            if (piece == W_PAWN)
            {
                game->en_passant_square = move.from + 8;  // White pawn moves up, so ep square is one rank above from
            }
            else
            {
                game->en_passant_square = move.from - 8;  // Black pawn moves down, so ep square is one rank below from
            }
        }
        else
        {
            // Clear en passant square if no double pawn push
            game->en_passant_square = -1;
        }
    }
    else
    {
        // Clear en passant square if move is not a pawn move
        game->en_passant_square = -1;
    }

    game->board[to] = (move.promotion) ? move.promotion_piece : piece;
    game->board[from] = EMPTY;

    if (move.is_en_passant)
    {
        int offset = (game->turn == WHITE) ? 8 : -8;

        game->board[game->en_passant_square + offset] = EMPTY;
    }

    if (move.is_castle_king_side)
    {
        if (move.piece == W_KING)
        {
            game->board[5] = W_ROOK;
            game->board[7] = EMPTY;
            
            game->has_white_castled = true;
        }
        else if (move.piece == B_KING)
        {
            game->board[61] = B_ROOK;
            game->board[63] = EMPTY;

            game->has_black_castled = true;
        }
    }
    else if (move.is_castle_queen_side)
    {
        if (move.piece == W_KING)
        {
            game->board[3] = W_ROOK;
            game->board[0] = EMPTY;

            game->has_white_castled = true;
        }
        else if (move.piece == B_KING)
        {
            game->board[59] = B_ROOK;
            game->board[56] = EMPTY;

            game->has_black_castled = true;
        }
    }

    generate_attack_tables(game, WHITE);
    generate_attack_tables(game, BLACK);

    game->turn ^= 1;  // toggle between WHITE (0) and BLACK (1)
}

void make_move_str(GameState *game, const char *move_str)
{
    if (!game || !move_str || strlen(move_str) < 4) return;

    int from = square_from_coords(&move_str[0]);
    int to   = square_from_coords(&move_str[2]);
    if (from < 0 || to < 0) return;

    Move move = {
        .from = from,
        .to = to,
        .piece = game->board[from],
        .promotion = false,
        .capture = (game->board[to] != EMPTY)
    };

    if (strlen(move_str) == 5) {
        move.promotion = true;
        char p = tolower(move_str[4]);
        switch (p) {
            case 'q': move.promotion_piece = W_QUEEN; break;
            case 'r': move.promotion_piece = W_ROOK; break;
            case 'b': move.promotion_piece = W_BISHOP; break;
            case 'n': move.promotion_piece = W_KNIGHT; break;
            default: return; // Invalid promotion
        }
    }

    make_move(game, move);
}