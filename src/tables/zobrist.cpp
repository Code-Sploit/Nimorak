#include <tables/zobrist.hpp>
#include <storage/transposition.hpp>
#include <core/board.hpp>

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace Nimorak {
    class Game;
}

namespace Zobrist {
    ZobristHash zobrist_pieces[NUM_PIECE_TYPES][NUM_SQUARES];
    ZobristHash zobrist_castling[NUM_CASTLING];
    ZobristHash zobrist_enpassant[NUM_ENPASSANT];
    ZobristHash zobrist_turn;

    const int piece_to_index[2][7] = {
        {1, 2, 3, 4, 5, 6},
        {7, 8, 9, 10, 11, 12}
    };

    // ----------------------------
    // Compute hash
    // ----------------------------
    ZobristHash compute(Nimorak::Game& game) {
        ZobristHash hash = 0ULL;

        Bitboard occupancy = game.occupancy[BOTH];

        while (occupancy) {
            int square = __builtin_ctzll(occupancy);

            Piece piece = game.boardGhost[square];
            int index = piece_to_index[Helpers::get_color(piece)][Helpers::get_type(piece)];

            hash ^= zobrist_pieces[index][square];

            occupancy &= occupancy - 1;
        }

        // Castling rights
        hash ^= zobrist_castling[game.castlingRights];

        // En passant
        if (game.enpassantSquare != -1) {
            int file = game.enpassantSquare & 7;
            hash ^= zobrist_enpassant[file];
        }

        // Side to move
        if (game.turn == BLACK) {
            hash ^= zobrist_turn;
        }

        return hash;
    }

    // ----------------------------
    // Update hash after a move
    // ----------------------------
    void updateMove(Nimorak::Game& game, Move move, Nimorak::State& oldState) {
        int from = Helpers::get_from(move);
        int to   = Helpers::get_to(move);
        int color = game.turn; // before flip

        ZobristHash hash = game.zobristKey;

        Piece moved_piece = game.boardGhost[from];

        // Captured piece
        Piece captured_piece = EMPTY;
        if (Helpers::is_enpassant(move)) {
            int ep_sq = (color == WHITE) ? (to - 8) : (to + 8);
            captured_piece = game.boardGhost[ep_sq];
        } else {
            captured_piece = game.boardGhost[to];
        }

        // Remove moved piece from `from`
        hash ^= zobrist_pieces[piece_to_index[Helpers::get_color(moved_piece)][Helpers::get_type(moved_piece)]][from];

        // Remove captured piece
        if (captured_piece != EMPTY) {
            int cap_sq = Helpers::is_enpassant(move) ? ((color == WHITE) ? to - 8 : to + 8) : to;
            hash ^= zobrist_pieces[piece_to_index[Helpers::get_color(captured_piece)][Helpers::get_type(captured_piece)]][cap_sq];
        }

        // Add moved/promo piece to `to`
        Piece piece_to_hash = moved_piece;
        if (Helpers::is_promo(move)) {
            piece_to_hash = Helpers::make_piece(Helpers::get_promo(move), color);
        }
        hash ^= zobrist_pieces[piece_to_index[Helpers::get_color(piece_to_hash)][Helpers::get_type(piece_to_hash)]][to];

        // Handle castling rook moves
        if (Helpers::is_castle(move)) {
            int rook_from = -1, rook_to = -1;
            if (color == WHITE) {
                if (to == 6) { rook_from = 7; rook_to = 5; }
                else if (to == 2) { rook_from = 0; rook_to = 3; }
            } else {
                if (to == 62) { rook_from = 63; rook_to = 61; }
                else if (to == 58) { rook_from = 56; rook_to = 59; }
            }
            if (rook_from != -1) {
                hash ^= zobrist_pieces[piece_to_index[color][ROOK]][rook_from];
                hash ^= zobrist_pieces[piece_to_index[color][ROOK]][rook_to];
            }
        }

        // Update castling rights
        hash ^= zobrist_castling[oldState.castlingRights];
        hash ^= zobrist_castling[game.castlingRights];

        // Update en passant
        if (oldState.enpassantSquare != -1) {
            int old_file = oldState.enpassantSquare & 7;
            hash ^= zobrist_enpassant[old_file];
        }
        if (game.enpassantSquare != -1) {
            int new_file = game.enpassantSquare & 7;
            hash ^= zobrist_enpassant[new_file];
        }

        // Toggle side to move
        hash ^= zobrist_turn;

        game.zobristKey = hash;
    }

    // ----------------------------
    // Update board (full recompute)
    // ----------------------------
    void updateBoard(Nimorak::Game& game) {
        game.zobristKey = compute(game);
    }

    // ----------------------------
    // Init keys
    // ----------------------------
    void init() {
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

    // ----------------------------
    // Hash to string
    // ----------------------------
    std::string hashToString(ZobristHash hash) {
        std::ostringstream oss;
        oss << "0x" << std::hex << std::setw(16) << std::setfill('0') << hash;
        return oss.str();
    }
} // namespace Zobrist