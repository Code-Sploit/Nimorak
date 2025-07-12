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
#include <stdio.h>

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

void board_load_fen(GameState *game, const char *fen)
{
    if (!game || !fen) return;

    board_clear(game);

    int idx = 56;

    // Parse piece placement
    for (; *fen && *fen != ' '; fen++) {
        if (*fen == '/')
        {
            idx -= 16;
        }
        else if (isdigit(*fen))
        {
            idx += *fen - '0';
        }
        else
        {
            game->board[idx++] = piece_from_char(*fen);
        }
    }

    // Skip space and parse turn
    if (*fen == ' ') fen++;
    game->turn = (*fen == 'w') ? WHITE : BLACK;

    // Skip turn char and space
    while (*fen && *fen != ' ') fen++;
    if (*fen == ' ') fen++;

    // Parse castling rights
    bool has_white_castle = false;
    bool has_black_castle = false;

    for (; *fen && *fen != ' '; fen++) {
        switch (*fen) {
            case 'K': has_white_castle = true; game->can_white_castle_kingside = true; break; // White kingside
            case 'Q': has_white_castle = true; game->can_white_castle_queenside = true; break; // White queenside
            case 'k': has_black_castle = true; game->can_black_castle_kingside = true; break; // Black kingside
            case 'q': has_black_castle = true; game->can_black_castle_queenside = true; break; // Black queenside
            case '-': break;
            default: break;
        }
    }

    game->permalock_white_castle = !has_white_castle;
    game->permalock_black_castle = !has_black_castle;
}

void board_movelist_clear(GameState *game)
{
    if (!game) return;

    memset(game->movelist, 0, sizeof(game->movelist));

    game->move_count = 0;
}

void board_movelist_add(GameState *game, Move move)
{
    if (!game || game->move_count >= MAX_LEGAL_MOVES) return;
    
    game->movelist[game->move_count++] = move;
}

void board_make_move(GameState *game, Move move)
{
    if (!game) return;

    int from = move.from, to = move.to;
    
    if ((unsigned)from > 63 || (unsigned)to > 63) return;

    int piece = game->board[from];

    if (move.is_en_passant)
    {
        int offset = (piece == W_PAWN) ? -8 : 8;

        game->board[game->en_passant_square + offset] = EMPTY;
    }

    switch (piece) {
        case W_ROOK:
            if (from == 0) game->can_white_castle_queenside = false;
            if (from == 7) game->can_white_castle_kingside = false;
            
            break;
        case W_KING:
            if (from == 4)
            {
                game->can_white_castle_kingside = false;
                game->can_white_castle_queenside = false;
                game->permalock_white_castle = true;
            }
            
            break;
        case B_ROOK:
            if (from == 56) game->can_black_castle_queenside = false;
            if (from == 63) game->can_black_castle_kingside = false;

            break;
        case B_KING:
            if (from == 60)
            {
                game->can_black_castle_kingside = false;
                game->can_black_castle_queenside = false;
                game->permalock_black_castle = true;
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

    //printf("From %d To %d EP %d\n", move.from, move.to, move.is_en_passant);

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

    attack_generate_table(game, WHITE);
    attack_generate_table(game, BLACK);

    game->turn ^= 1;  // toggle between WHITE (0) and BLACK (1)
}

void board_make_move_str(GameState *game, const char *move_str)
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
            case 'q': move.promotion_piece = (game->turn == WHITE) ? W_QUEEN : B_QUEEN; break;
            case 'r': move.promotion_piece = (game->turn == WHITE) ? W_ROOK : B_ROOK; break;
            case 'b': move.promotion_piece = (game->turn == WHITE) ? W_BISHOP : B_BISHOP; break;
            case 'n': move.promotion_piece = (game->turn == WHITE) ? W_KNIGHT : B_KNIGHT; break;
            default: return; // Invalid promotion
        }
    }

    board_make_move(game, move);
}

void board_clear(GameState *game)
{
    if (!game) return;

    for (int i = 0; i < 64; i++)
    {
        game->board[i] = EMPTY;
    }
}

void board_print(GameState *game)
{
    printf("  +---+---+---+---+---+---+---+---+\n");

    for (int rank = 7; rank >= 0; rank--)
    {
        printf("%d |", rank + 1);

        for (int file = 0; file < 8; file++)
        {
            int sq = rank * 8 + file;

            printf(" %s |", piece_to_char(game->board[sq]));
        }

        printf("\n  +---+---+---+---+---+---+---+---+\n");
    }

    printf("    a   b   c   d   e   f   g   h\n");
}