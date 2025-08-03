#include "zobrist.h"

#include <stdlib.h>
#include <time.h>

ZobristHash zobrist_pieces[NUM_PIECE_TYPES][NUM_SQUARES];
ZobristHash zobrist_castling[NUM_CASTLING];
ZobristHash zobrist_enpassant[NUM_ENPASSANT];
ZobristHash zobrist_turn;

static inline int piece_to_index(int color, int piece)
{
    if (color == WHITE) return piece;

    return piece + 6;
}

ZobristHash zobrist_compute_hash(Game *game)
{
    ZobristHash hash = 0ULL;

    for (int color = WHITE; color <= BLACK; color++)
    {
        for (int piece = PAWN; piece <= KING; piece++)
        {
            Bitboard positions = game->board[color][piece];

            while (positions)
            {
                int square = __builtin_ctzll(positions);

                hash ^= zobrist_pieces[piece_to_index(color, piece)][square];

                positions &= positions - 1;
            }
        }
    }

    hash ^= zobrist_castling[game->castling_rights];

    if (game->enpassant_square != -1)
    {
        int file = game->enpassant_square & 8;

        hash ^= zobrist_enpassant[file];
    }

    if (game->turn == BLACK)
    {
        hash ^= zobrist_turn;
    }

    return hash;
}

void zobrist_init()
{
    srand(12345678);

    for (int p = 0; p < NUM_PIECE_TYPES; p++)
        for (int sq = 0; sq < NUM_SQUARES; sq++)
            zobrist_pieces[p][sq] = ((uint64_t)rand() << 32) | rand();

    for (int i = 0; i < NUM_CASTLING; i++)
        zobrist_castling[i] = ((uint64_t)rand() << 32) | rand();

    for (int i = 0; i < NUM_ENPASSANT; i++)
        zobrist_enpassant[i] = ((uint64_t)rand() << 32) | rand();

    zobrist_turn = ((uint64_t)rand() << 32) | rand();
}

void zobrist_update_board(Game *game)
{
    if (!game) return;

    game->zobrist_key = zobrist_compute_hash(game);
}