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

void zobrist_update_move(Game *game, Move move, State *old_state) {
    int from = GET_FROM(move);
    int to = GET_TO(move);
    int color = game->turn;  // side to move BEFORE flipping turn

    ZobristHash hash = game->zobrist_key;  // start from current hash for XOR delta

    Piece moved_piece = board_get_square(game, from);

    // Captured piece (including en passant special case)
    Piece captured_piece = EMPTY;
    if (IS_ENPASSANT(move)) {
        int ep_capture_sq = (color == WHITE) ? (to - 8) : (to + 8);
        captured_piece = board_get_square(game, ep_capture_sq);
    } else {
        captured_piece = board_get_square(game, to);
    }

    // 1) XOR out moved piece from 'from' square
    hash ^= zobrist_pieces[piece_to_index[GET_COLOR(moved_piece)][GET_TYPE(moved_piece)]][from];

    // 2) XOR out captured piece from capture square (if any)
    if (captured_piece != EMPTY) {
        int cap_sq = IS_ENPASSANT(move) ? ((color == WHITE) ? to - 8 : to + 8) : to;
        hash ^= zobrist_pieces[piece_to_index[GET_COLOR(captured_piece)][GET_TYPE(captured_piece)]][cap_sq];
    }

    // 3) XOR in moved piece on 'to' square (handle promotions)
    Piece piece_to_hash = moved_piece;
    if (IS_PROMO(move)) {
        piece_to_hash = MAKE_PIECE(GET_PROMO(move), color);
    }
    hash ^= zobrist_pieces[piece_to_index[GET_COLOR(piece_to_hash)][GET_TYPE(piece_to_hash)]][to];

    // 4) Handle castling rook moves
    if (IS_CASTLE(move)) {
        int rook_from = -1, rook_to = -1;
        if (color == WHITE) {
            if (to == 6) {       // White kingside castle
                rook_from = 7; 
                rook_to = 5;
            } else if (to == 2) { // White queenside castle
                rook_from = 0;
                rook_to = 3;
            }
        } else {
            if (to == 62) {       // Black kingside castle
                rook_from = 63;
                rook_to = 61;
            } else if (to == 58) { // Black queenside castle
                rook_from = 56;
                rook_to = 59;
            }
        }
        if (rook_from != -1 && rook_to != -1) {
            // XOR out rook from old square
            hash ^= zobrist_pieces[piece_to_index[color][ROOK]][rook_from];
            // XOR in rook on new square
            hash ^= zobrist_pieces[piece_to_index[color][ROOK]][rook_to];
        }
    }

    // 5) XOR out old castling rights and XOR in new castling rights
    hash ^= zobrist_castling[old_state->castling_rights];
    hash ^= zobrist_castling[game->castling_rights];

    // 6) XOR out old en passant file and XOR in new en passant file (if any)
    if (old_state->enpassant_square != -1) {
        int old_file = old_state->enpassant_square & 7;
        hash ^= zobrist_enpassant[old_file];
    }
    if (game->enpassant_square != -1) {
        int new_file = game->enpassant_square & 7;
        hash ^= zobrist_enpassant[new_file];
    }

    // 7) XOR side to move (always toggle)
    hash ^= zobrist_turn;

    // 8) Update the game hash key
    game->zobrist_key = hash;
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