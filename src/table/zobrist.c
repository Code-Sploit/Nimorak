#include <table/transposition.h>
#include <table/zobrist.h>

#include <board/board.h>

#include <stdlib.h>
#include <time.h>

ZobristHash zobrist_pieces[NUM_PIECE_TYPES][NUM_SQUARES];
ZobristHash zobrist_castling[NUM_CASTLING];
ZobristHash zobrist_enpassant[NUM_ENPASSANT];
ZobristHash zobrist_turn;

const int piece_to_index[2][7] = {
    {1, 2, 3, 4, 5, 6},
    {7, 8, 9, 10, 11, 12}
};

ZobristHash zobrist_compute_hash(Game *game)
{
    ZobristHash hash = 0ULL;

    Bitboard occupancy = game->occupancy[BOTH];

    while (occupancy)
    {
        int square = __builtin_ctzll(occupancy);

        Piece piece = board_get_square(game, square);

        int index = piece_to_index[GET_COLOR(piece)][GET_TYPE(piece)];

        hash ^= zobrist_pieces[index][square];

        occupancy &= occupancy - 1;
    }

    // Castling rights
    hash ^= zobrist_castling[game->castling_rights];

    // En passant (file-based hashing only)
    if (game->enpassant_square != -1)
    {
        int file = game->enpassant_square & 7; // safer than % 8
        hash ^= zobrist_enpassant[file];
    }

    // Side to move
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