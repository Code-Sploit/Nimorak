#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "helper.h"

char *translate_square_to_string(int square) {
    static char buffers[8][3];  // Up to 8 simultaneous calls
    static int index = 0;

    index = (index + 1) % 8;  // Cycle through 0–7

    int file = square % 8;
    int rank = square / 8;

    buffers[index][0] = 'a' + file;
    buffers[index][1] = '1' + rank;
    buffers[index][2] = '\0';

    return buffers[index];
}

int piece_from_char(char c)
{
    switch (c) {
        case 'P': return W_PAWN;
        case 'N': return W_KNIGHT;
        case 'B': return W_BISHOP;
        case 'R': return W_ROOK;
        case 'Q': return W_QUEEN;
        case 'K': return W_KING;
        case 'p': return B_PAWN;
        case 'n': return B_KNIGHT;
        case 'b': return B_BISHOP;
        case 'r': return B_ROOK;
        case 'q': return B_QUEEN;
        case 'k': return B_KING;
        default: return EMPTY;
    }
}

int is_color(int color, int turn)
{
    if (color == 0) return false;
    if (turn == WHITE && color > 0) return true;
    if (turn == BLACK && color < 0) return true;

    return false;
}

int is_enemy_piece(int color, int turn)
{
    if (color == 0) return false;
    if (is_color(color, turn) == true) return false;

    return true;
}

int square_from_string(const char *sq_str)
{
    int file = file_to_index(sq_str[0]);
    int rank = rank_to_index(sq_str[1]);

    return rank * 8 + file;
}

int file_to_index(char file)
{
    return file - 'a';
}

int rank_to_index(char rank)
{
    return rank - '1';
}

void parse_move_str(const char *move_str, int *from_sq, int *to_sq)
{
    *from_sq = square_from_string(move_str);
    *to_sq = square_from_string(move_str + 2);
}

int find_king(GameState *game, int color)
{
    if (!game) return -1;

    for (int i = 0; i < 64; i++)
    {
        int piece = game->board[i];

        if (piece == W_KING && color == WHITE) return i;
        if (piece == B_KING && color == BLACK) return i;
    }

    return -1;
}

int is_king_in_check(GameState *game, int color)
{
    if (!game) return -1;

    int king_square = find_king(game, color);

    return is_square_attacked_by(game, king_square, (color == WHITE) ? BLACK : WHITE);
}

int get_square(GameState *game, int square)
{
    if (!game) return -1;

    return game->board[square];
}

int is_square_attacked_by(GameState *game, int square, int color)
{
    if (!game) return -1;

    if (color == WHITE)
    {
        return (game->white_controlled_squares[square] == true) ? true : false;
    }
    else if (color == BLACK)
    {
        return (game->black_controlled_squares[square] == true) ? true : false;
    }

    return -1;
}

const char* piece_to_char(int piece) {
    switch (piece) {
        case W_PAWN:   return "P";
        case W_KNIGHT: return "N";
        case W_BISHOP: return "B";
        case W_ROOK:   return "R";
        case W_QUEEN:  return "Q";
        case W_KING:   return "K";
        case B_PAWN:   return "p";
        case B_KNIGHT: return "n";
        case B_BISHOP: return "b";
        case B_ROOK:   return "r";
        case B_QUEEN:  return "q";
        case B_KING:   return "k";
        default: return ".";
    }
}

int can_king_castle(GameState *game, int color, int side)
{
    if (!game) return -1;

    if (is_king_in_check(game, color)) return false;

    if (side == QUEEN_SIDE && color == WHITE)
    {
        if (is_square_attacked_by(game, 2, BLACK) || is_square_attacked_by(game, 3, BLACK)) return false;
        if (get_square(game, 1) != 0 || get_square(game, 2) != 0 || get_square(game, 3) != 0) return false;
        if (get_square(game, 0) != W_ROOK) return false;
    }
    else if (side == QUEEN_SIDE && color == BLACK)
    {
        if (is_square_attacked_by(game, 58, WHITE) || is_square_attacked_by(game, 59, WHITE)) return false;
        if (get_square(game, 57) != 0 || get_square(game, 58) != 0 || get_square(game, 59) != 0) return false;
        if (get_square(game, 56) != B_ROOK) return false;
    }
    else if (side == KING_SIDE && color == BLACK)
    {
        if (is_square_attacked_by(game, 61, WHITE) || is_square_attacked_by(game, 62, WHITE)) return false;
        if (get_square(game, 61) != 0 || get_square(game, 62) != 0) return false;
        if (get_square(game, 63) != B_ROOK) return false;
    }
    else if (side == KING_SIDE && color == WHITE)
    {
        if (is_square_attacked_by(game, 5, BLACK) || is_square_attacked_by(game, 6, BLACK)) return false;
        if (get_square(game, 5) != 0 || get_square(game, 6) != 0) return false;
        if (get_square(game, 7) != W_ROOK) return false;
    }
    else
    {
        return -1;
    }

    return -1;
}

int square_from_coords(const char* coords) {
    if (coords[0] < 'a' || coords[0] > 'h') return -1;
    if (coords[1] < '1' || coords[1] > '8') return -1;
    int file = coords[0] - 'a';
    int rank = coords[1] - '1';
    return rank * 8 + file;
}

char get_promotion_piece(Move move)
{
    if (!move.promotion) return ' ';

    switch (move.promotion_piece)
    {
        case W_KNIGHT:
        case B_KNIGHT: return 'n';
        case W_BISHOP:
        case B_BISHOP: return 'b';
        case W_ROOK:
        case B_ROOK: return 'r';
        case W_QUEEN:
        case B_QUEEN: return 'q';
        default: return ' ';
    }

    return ' ';
}

void print_board(GameState *game)
{
    printf("  +---+---+---+---+---+---+---+---+\n");
    for (int rank = 7; rank >= 0; rank--) {
        printf("%d |", rank + 1);
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            printf(" %s |", piece_to_char(game->board[sq]));
        }
        printf("\n  +---+---+---+---+---+---+---+---+\n");
    }
    printf("    a   b   c   d   e   f   g   h\n");
}

int piece_value(int piece) {
    switch (piece) {
        case W_PAWN: case B_PAWN: return 100;
        case W_KNIGHT: case B_KNIGHT: return 300;
        case W_BISHOP: case B_BISHOP: return 330;
        case W_ROOK: case B_ROOK: return 500;
        case W_QUEEN: case B_QUEEN: return 900;
        case W_KING: case B_KING: return 100000;
        default: return 0;
    }
}

void clear_board(GameState *game)
{
    if (!game) return;

    for (int i = 0; i < 64; i++)
    {
        game->board[i] = EMPTY;
    }
}