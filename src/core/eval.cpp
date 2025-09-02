#include <core/eval.hpp>
#include <core/board.hpp>
#include <core/nimorak.hpp>

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <iostream>

namespace Nimorak {
    class Game;
}

const int piece_values[] = {0, 100, 300, 350, 500, 900};

const int eval_pawn_structure_penalties[9] = {
    0,   // 0 pawns on file
    0,   // 1 pawn on file = fine
    20,  // doubled
    50,  // tripled
    90,  // quadrupled
    140, // etc.
    200,
    280,
    360
};

static const Bitboard FILE_MASKS[8] = {
    0x0101010101010101ULL,
    0x0202020202020202ULL,
    0x0404040404040404ULL,
    0x0808080808080808ULL,
    0x1010101010101010ULL,
    0x2020202020202020ULL,
    0x4040404040404040ULL,
    0x8080808080808080ULL
};

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

static inline double eval_calculate_endgame_weight(Nimorak::Game& game)
{
    int total_material = 0;
    int max_material = 40; // approximate

    for (int color = 0; color < 2; color++) {
        total_material += 9 * __builtin_popcountll(game.board[color][QUEEN]);
        total_material += 5 * __builtin_popcountll(game.board[color][ROOK]);
        total_material += 3 * __builtin_popcountll(game.board[color][BISHOP]);
        total_material += 3 * __builtin_popcountll(game.board[color][KNIGHT]);
        total_material += 1 * __builtin_popcountll(game.board[color][PAWN]);
    }

    double weight = 1.0 - (static_cast<double>(total_material) / max_material);
    weight = std::clamp(weight, 0.0, 1.0);
    return weight;
}

// --- Eval modules ---
void eval_module_material(void* arg)
{
    Nimorak::Game& game = *static_cast<Nimorak::Game*>(arg);
    int eval = 0;
    Bitboard occupancy = game.occupancy[BOTH];

    while (occupancy) {
        int sq = __builtin_ctzll(occupancy);
        occupancy &= occupancy - 1;

        Piece p = game.boardGhost[sq];
        int type = Helpers::get_type(p);
        int color = Helpers::get_color(p);
        int perspective = (color == WHITE) ? 1 : -1;

        eval += piece_values[type] * perspective;
    }

    game.eval += eval;
}

void eval_module_pst(void* arg)
{
    Nimorak::Game& game = *static_cast<Nimorak::Game*>(arg);
    int eval = 0;
    double endgame_weight = eval_calculate_endgame_weight(game);

    Bitboard occupancy = game.occupancy[BOTH];
    while (occupancy) {
        int sq = __builtin_ctzll(occupancy);
        occupancy &= occupancy - 1;

        Piece p = game.boardGhost[sq];
        int type = Helpers::get_type(p);
        int color = Helpers::get_color(p);
        int perspective = (color == WHITE) ? 1 : -1;
        int real_sq = (color == BLACK) ? mirror[sq] : sq;

        if (type == KING) {
            int king_val = static_cast<int>((1.0 - endgame_weight) * eval_king_mid_pst[real_sq] +
                                            endgame_weight * eval_king_end_pst[real_sq]);
            eval += king_val * perspective;
        } else {
            eval += eval_pst_table[type][real_sq] * perspective;
        }
    }

    game.eval += eval;
}

void eval_module_king_safety(void* arg)
{
    Nimorak::Game& game = *static_cast<Nimorak::Game*>(arg);
    int eval = 0;

    for (int color = 0; color < 2; color++) {
        int king_sq = Board::findKing(game, color);
        Bitboard front = 0ULL;
        int rank = Helpers::rank_of(king_sq);

        if (color == WHITE) {
            if (rank < 7) front |= (1ULL << (king_sq + 8));
            if (rank < 6) front |= (1ULL << (king_sq + 16));
        } else {
            if (rank > 0) front |= (1ULL << (king_sq - 8));
            if (rank > 1) front |= (1ULL << (king_sq - 16));
        }

        int shield_pawns = __builtin_popcountll(front & game.board[color][PAWN]);
        eval += (shield_pawns - 2) * ((color == WHITE) ? 15 : -15);
    }

    game.eval += eval;
}

void eval_module_pawn_structure(void* arg)
{
    Nimorak::Game& game = *static_cast<Nimorak::Game*>(arg);
    int eval = 0;

    for (int color = 0; color < 2; color++) {
        for (int file = 0; file < 8; file++) {
            int count = 0;
            for (int rank = 0; rank < 8; rank++) {
                int sq = rank * 8 + file;
                if (Helpers::get_type(game.boardGhost[sq]) == PAWN &&
                    Helpers::get_color(game.boardGhost[sq]) == color) count++;
            }

            if (count > 1) eval += (count - 1) * ((color == WHITE) ? -20 : 20);
        }
    }

    game.eval += eval;
}

// Mobility: number of legal moves
void eval_module_mobility(void *arg)
{
    Nimorak::Game& game = *static_cast<Nimorak::Game*>(arg);
    int eval = 0;

    for (int color = 0; color < 2; color++) {
        int mobility = 0;
        for (int piece = KNIGHT; piece <= QUEEN; piece++) {
            Bitboard pcs = game.board[color][piece];
            while (pcs) {
                int sq = __builtin_ctzll(pcs);
                pcs &= pcs - 1;
                mobility += __builtin_popcountll(game.attackWorker.attackMap[color][sq]);
            }
        }
        eval += ((color == WHITE) ? mobility : -mobility) * 2;
    }

    game.eval += eval;
}

// Endgame king centralization
void eval_module_endgame(void *arg)
{
    Nimorak::Game& game = *static_cast<Nimorak::Game*>(arg);
    int eval = 0;
    double endgame_weight = eval_calculate_endgame_weight(game);
    if (endgame_weight < 0.1) return; // skip in opening/midgame

    int center_squares[4] = {27,28,35,36};
    int friendly_king_sq = Board::findKing(game, game.turn);
    int enemy_king_sq    = Board::findKing(game, !game.turn);

    int min_dist_friendly = 1000, min_dist_enemy = 1000;
    for (int i = 0; i < 4; i++) {
        int sq = center_squares[i];
        int fd = abs(Helpers::rank_of(friendly_king_sq) - Helpers::rank_of(sq)) +
                 abs(Helpers::file_of(friendly_king_sq) - Helpers::file_of(sq));
        int ed = abs(Helpers::rank_of(enemy_king_sq) - Helpers::rank_of(sq)) +
                 abs(Helpers::file_of(enemy_king_sq) - Helpers::file_of(sq));
        if (fd < min_dist_friendly) min_dist_friendly = fd;
        if (ed < min_dist_enemy) min_dist_enemy = ed;
    }

    eval += (6 - min_dist_friendly) * endgame_weight * 10;
    eval -= (6 - min_dist_enemy) * endgame_weight * 10;

    game.eval += eval;
}

// Rook open files
void eval_module_rook_open_files(void *arg)
{
    Nimorak::Game& game = *static_cast<Nimorak::Game*>(arg);
    int eval = 0;

    static const int open_file_bonus = 25;
    static const int semi_open_file_bonus = 15;

    for (int color = WHITE; color <= BLACK; color++)
    {
        Bitboard rooks = game.board[color][ROOK];

        while (rooks)
        {
            int square = __builtin_ctzll(rooks);
            rooks &= rooks - 1;

            int file = Helpers::file_of(square);
            Bitboard file_mask = FILE_MASKS[file];

            Bitboard friendly_pawns = file_mask & game.board[color][PAWN];
            Bitboard enemy_pawns = file_mask & game.board[color ^ 1][PAWN];

            if (!friendly_pawns && !enemy_pawns)
                eval += (color == WHITE) ? open_file_bonus : -open_file_bonus;
            else if (!friendly_pawns && enemy_pawns)
                eval += (color == WHITE) ? semi_open_file_bonus : -semi_open_file_bonus;
        }
    }

    game.eval += eval;
}

void eval_module_bishop_pair(void *arg)
{
    Nimorak::Game& game = *static_cast<Nimorak::Game*>(arg);
    int eval = 0;
    const int bishop_pair_bonus = 30;

    for (int color = WHITE; color <= BLACK; color++)
    {
        int count = __builtin_popcountll(game.board[color][BISHOP]);
        if (count >= 2)
            eval += (color == WHITE) ? bishop_pair_bonus : -bishop_pair_bonus;
    }

    game.eval += eval;
}

void eval_module_knight_outposts(void *arg)
{
    Nimorak::Game& game = *static_cast<Nimorak::Game*>(arg);
    int eval = 0;
    const int outpost_bonus = 20;

    // central ranks mask (ranks 3..6)
    const Bitboard central_mask = 0x00003C3C3C3C0000ULL;

    for (int color = WHITE; color <= BLACK; color++)
    {
        Bitboard knights = game.board[color][KNIGHT];

        while (knights)
        {
            int square = __builtin_ctzll(knights);
            knights &= knights - 1;

            Bitboard sq_mask = 1ULL << square;

            // Only consider central squares
            if (!(sq_mask & central_mask))
                continue;

            // Pawns protecting the square
            Bitboard friendly_pawns = game.board[color][PAWN];
            Bitboard enemy_pawns = game.board[color ^ 1][PAWN];

            int file = Helpers::file_of(square);

            Bitboard protected_by_pawn = 0ULL;

            if (color == WHITE)
            {
                if (file > 0) protected_by_pawn |= 1ULL << (square - 9);
                if (file < 7) protected_by_pawn |= 1ULL << (square - 7);
            }
            else
            {
                if (file > 0) protected_by_pawn |= 1ULL << (square + 7);
                if (file < 7) protected_by_pawn |= 1ULL << (square + 9);
            }

            if ((protected_by_pawn & friendly_pawns) && !(protected_by_pawn & enemy_pawns))
            {
                eval += (color == WHITE) ? outpost_bonus : -outpost_bonus;
            }
        }
    }

    game.eval += eval;
}

namespace Evaluation {
    // --- Eval entry point ---
    int Worker::evaluate(Nimorak::Game& game)
    {
        game.eval = 0;
        game.evalModuleList.run();
        return (game.turn == WHITE) ? game.eval : -game.eval;
    }

    // --- Eval init / quit / reinit ---
    void Worker::init(Nimorak::Game& game)
    {
        int size = 0;
        if (game.config.eval.doMaterial)         size++;
        if (game.config.eval.doPieceSquares)    size++;
        if (game.config.eval.doEndgame)          size++;
        if (game.config.eval.doMobility)         size++;
        if (game.config.eval.doPawnStructure)   size++;
        if (game.config.eval.doRookOpenFiles)  size++;
        if (game.config.eval.doKingSafety)      size++;
        if (game.config.eval.doBishopPair)      size++;
        if (game.config.eval.doKnightOutposts)  size++;

        game.evalModuleList.init(size);

        if (game.config.eval.doMaterial)         game.evalModuleList.add(eval_module_material, &game, "eval_module_material");
        if (game.config.eval.doPieceSquares)    game.evalModuleList.add(eval_module_pst, &game, "eval_module_pst");
        if (game.config.eval.doEndgame)          game.evalModuleList.add(eval_module_endgame, &game, "eval_module_endgame");
        if (game.config.eval.doMobility)         game.evalModuleList.add(eval_module_mobility, &game, "eval_module_mobility");
        if (game.config.eval.doKingSafety)      game.evalModuleList.add(eval_module_king_safety, &game, "eval_module_king_safety");
        if (game.config.eval.doPawnStructure)   game.evalModuleList.add(eval_module_pawn_structure, &game, "eval_module_pawn_structure");
        if (game.config.eval.doRookOpenFiles)  game.evalModuleList.add(eval_module_rook_open_files, &game, "eval_module_rook_open_files");
        if (game.config.eval.doBishopPair)      game.evalModuleList.add(eval_module_bishop_pair, &game, "eval_module_bishop_pair");
        if (game.config.eval.doKnightOutposts)  game.evalModuleList.add(eval_module_knight_outposts, &game, "eval_module_knight_outposts");
    }

    void Worker::quit(Nimorak::Game& game)
    {
        game.evalModuleList.free();
    }

    void Worker::reinit(Nimorak::Game& game)
    {
        quit(game);
        init(game);
    }
}