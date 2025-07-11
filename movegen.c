#include "movegen.h"
#include "attack.h"
#include "board.h"
#include "helper.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

void generate_legal_moves(GameState *game)
{
    generate_pseudo_moves(game);

    generate_attack_tables(game, WHITE);
    generate_attack_tables(game, BLACK);

    Move legal_moves[MAX_LEGAL_MOVES];

    int legal_move_count = 0;

    for (int i = 0; i < game->move_count; i++)
    {
        GameState copy;

        clone_game_state(&copy, game);

        make_move(&copy, game->movelist[i]);

        if (!is_king_in_check(&copy, game->turn))
        {
            legal_moves[legal_move_count] = game->movelist[i];
            legal_move_count++;
        }
        else
        {
            //printf("King is in check after move %s%s\n", translate_square_to_string(game->movelist[i].from), translate_square_to_string(game->movelist[i].to));
            //printf("illegal %s%s\n", translate_square_to_string(game->movelist[i].from), translate_square_to_string(game->movelist[i].to));
        }
    }

    for (int i = 0; i < legal_move_count; i++)
    {
        game->movelist[i] = legal_moves[i];
    }

    game->move_count = legal_move_count;
}

void generate_pseudo_moves(GameState *game)
{
    if (!game) return;

    clear_move_list(game);
    clear_attack_tables(game, game->turn);

    for (int i = 0; i < 64; i++)
    {
        int piece = game->board[i];

        int file = i % 8;
        int rank = i / 8;

        if (is_color(piece, game->turn) == false) continue;

        switch (piece) {
            case W_PAWN:
            {
                int start_file = i % 8;
                int start_rank = i / 8;

                int target_one_step = i + PAWN_OFFSETS[0];
                int target_two_step = i + PAWN_OFFSETS[1];
                int target_capture_left = i + PAWN_OFFSETS[4];
                int target_capture_right = i + PAWN_OFFSETS[5];

                bool is_on_second_rank = (start_rank == 1);
                bool is_on_seventh_rank = (start_rank == 6); // promotion rank after move

                // Check if target squares are on board (0-63)
                bool valid_one_step = (target_one_step >= 0 && target_one_step < 64);
                bool valid_two_step = (target_two_step >= 0 && target_two_step < 64);
                bool valid_capture_left = (target_capture_left >= 0 && target_capture_left < 64);
                bool valid_capture_right = (target_capture_right >= 0 && target_capture_right < 64);

                // Normal move
                if (valid_one_step && game->board[target_one_step] == EMPTY) {
                    if (is_on_seventh_rank) {
                        add_move(game, (Move){.from = i, .to = target_one_step, .promotion = true, .capture = false, .piece = W_PAWN, .promotion_piece = W_QUEEN});
                        add_move(game, (Move){.from = i, .to = target_one_step, .promotion = true, .capture = false, .piece = W_PAWN, .promotion_piece = W_ROOK});
                        add_move(game, (Move){.from = i, .to = target_one_step, .promotion = true, .capture = false, .piece = W_PAWN, .promotion_piece = W_BISHOP});
                        add_move(game, (Move){.from = i, .to = target_one_step, .promotion = true, .capture = false, .piece = W_PAWN, .promotion_piece = W_KNIGHT});
                    } else {
                        add_move(game, (Move){.from = i, .to = target_one_step, .promotion = false, .capture = false, .piece = W_PAWN});
                    }

                    // Double push only if first step free & on second rank
                    if (valid_two_step && is_on_second_rank && game->board[target_two_step] == EMPTY) {
                        add_move(game, (Move){.from = i, .to = target_two_step, .promotion = false, .capture = false, .piece = W_PAWN});
                    }
                }

                // Capture right (make sure target file is exactly one right)
                if (valid_capture_right) {
                    int target_file = target_capture_right % 8;
                    if (target_file == start_file + 1 && is_enemy_piece(game->board[target_capture_right], game->turn)) {
                        if (is_on_seventh_rank) {
                            add_move(game, (Move){.from = i, .to = target_capture_right, .promotion = true, .capture = true, .piece = W_PAWN, .promotion_piece = W_QUEEN});
                            add_move(game, (Move){.from = i, .to = target_capture_right, .promotion = true, .capture = true, .piece = W_PAWN, .promotion_piece = W_ROOK});
                            add_move(game, (Move){.from = i, .to = target_capture_right, .promotion = true, .capture = true, .piece = W_PAWN, .promotion_piece = W_BISHOP});
                            add_move(game, (Move){.from = i, .to = target_capture_right, .promotion = true, .capture = true, .piece = W_PAWN, .promotion_piece = W_KNIGHT});
                        } else {
                            add_move(game, (Move){.from = i, .to = target_capture_right, .promotion = false, .capture = true, .piece = W_PAWN});
                        }
                    }
                }

                // Capture left (make sure target file is exactly one left)
                if (valid_capture_left) {
                    int target_file = target_capture_left % 8;
                    if (target_file == start_file - 1 && is_enemy_piece(game->board[target_capture_left], game->turn)) {
                        if (is_on_seventh_rank) {
                            add_move(game, (Move){.from = i, .to = target_capture_left, .promotion = true, .capture = true, .piece = W_PAWN, .promotion_piece = W_QUEEN});
                            add_move(game, (Move){.from = i, .to = target_capture_left, .promotion = true, .capture = true, .piece = W_PAWN, .promotion_piece = W_ROOK});
                            add_move(game, (Move){.from = i, .to = target_capture_left, .promotion = true, .capture = true, .piece = W_PAWN, .promotion_piece = W_BISHOP});
                            add_move(game, (Move){.from = i, .to = target_capture_left, .promotion = true, .capture = true, .piece = W_PAWN, .promotion_piece = W_KNIGHT});
                        } else {
                            add_move(game, (Move){.from = i, .to = target_capture_left, .promotion = false, .capture = true, .piece = W_PAWN});
                        }
                    }
                }

                if (game->en_passant_square != -1) {
                    int ep_square = game->en_passant_square;
                    int ep_file = ep_square % 8;
                    int ep_rank = ep_square / 8;

                    // White pawn must be on rank 4 (index 4) to capture en passant
                    if (start_rank == 4) {
                        // The pawn to be captured is behind the en passant square (rank 3)
                        int captured_pawn_square = ep_square - 8;  // one rank below ep square

                        // Check if there is actually a black pawn on the captured pawn square
                        if (game->board[captured_pawn_square] == B_PAWN) {

                            // En passant capture to the left
                            if (ep_square == target_capture_left && ep_file == start_file - 1) {
                                add_move(game, (Move){
                                    .from = i,
                                    .to = ep_square,
                                    .promotion = false,
                                    .capture = true,
                                    .piece = W_PAWN,
                                    .is_en_passant = true
                                });
                            }
                            // En passant capture to the right
                            else if (ep_square == target_capture_right && ep_file == start_file + 1) {
                                add_move(game, (Move){
                                    .from = i,
                                    .to = ep_square,
                                    .promotion = false,
                                    .capture = true,
                                    .piece = W_PAWN,
                                    .is_en_passant = true
                                });
                            }
                        }
                    }
                }

                break;
            }

            case W_KNIGHT:
            {
                for (int j = 0; j < 8; j++) {
                    int new_square = i + KNIGHT_OFFSETS[j];

                    if (new_square < 0 || new_square > 63) continue;

                    int new_file = new_square % 8;
                    int new_rank = new_square / 8;

                    // Prevent wraparound (e.g. jumping from file H to file A)
                    if (abs(new_file - file) > 2 || abs(new_rank - rank) > 2) continue;

                    if (is_color(game->board[new_square], game->turn)) continue;

                    Move move = {
                        .from = i,
                        .to = new_square,
                        .promotion = false,
                        .capture = is_enemy_piece(game->board[new_square], game->turn),
                        .piece = W_KNIGHT
                    };

                    add_move(game, move);
                }

                break;
            }
            case W_BISHOP:
            {
                for (int j = 0; j < 4; j++) { // BISHOP_OFFSETS should have 4 directions: NE, NW, SE, SW
                    int offset = BISHOP_OFFSETS[j];
                    int target = i;

                    while (true) {
                        int prev_file = target % 8;
                        int prev_rank = target / 8;

                        target += offset;

                        if (target < 0 || target > 63) break;

                        int target_file = target % 8;
                        int target_rank = target / 8;

                        // If not moving diagonally or wrapped around file boundary, stop
                        if (abs(target_file - prev_file) != 1 || abs(target_rank - prev_rank) != 1) break;

                        if (is_color(game->board[target], game->turn)) break;

                        Move move = {
                            .from = i,
                            .to = target,
                            .promotion = false,
                            .capture = is_enemy_piece(game->board[target], game->turn),
                            .piece = W_BISHOP
                        };

                        add_move(game, move);

                        if (move.capture) break;
                    }
                }

                break;
            }
            case W_ROOK:
            {
                for (int j = 0; j < 4; j++) { // ROOK_OFFSETS: +1 (→), -1 (←), +8 (↑), -8 (↓)
                    int offset = ROOK_OFFSETS[j];
                    int target = i;

                    while (true) {
                        int prev_file = target % 8;
                        int prev_rank = target / 8;

                        target += offset;

                        if (target < 0 || target > 63) break;

                        int target_file = target % 8;
                        int target_rank = target / 8;

                        // Prevent wrapping in horizontal directions
                        if ((offset == 1 || offset == -1) && target_rank != prev_rank) break;

                        if (is_color(game->board[target], game->turn)) break;

                        Move move = {
                            .from = i,
                            .to = target,
                            .promotion = false,
                            .capture = is_enemy_piece(game->board[target], game->turn),
                            .piece = W_ROOK
                        };

                        add_move(game, move);

                        if (move.capture) break;
                    }
                }

                break;
            }
            case W_QUEEN:
            {
                for (int j = 0; j < 8; j++) { // QUEEN_OFFSETS: combines ROOK and BISHOP directions
                    int offset = QUEEN_OFFSETS[j];
                    int target = i;

                    while (true) {
                        int prev_file = target % 8;
                        int prev_rank = target / 8;

                        target += offset;
                        if (target < 0 || target > 63) break;

                        int target_file = target % 8;
                        int target_rank = target / 8;

                        // Prevent file/rank wraparounds
                        if ((offset == 1 || offset == -1) && target_rank != prev_rank) break;
                        if ((offset == 8 || offset == -8) && target_file != prev_file) break;
                        if ((offset == 7 || offset == -7 || offset == 9 || offset == -9) &&
                            abs(target_file - prev_file) != 1) break;

                        if (is_color(game->board[target], game->turn)) break;

                        Move move = {
                            .from = i,
                            .to = target,
                            .promotion = false,
                            .capture = is_enemy_piece(game->board[target], game->turn),
                            .piece = W_QUEEN
                        };

                        add_move(game, move);

                        if (move.capture) break;
                    }
                }

                break;
            }
            case W_KING:
            {
                for (int j = 0; j < 8; j++) {
                    int offset = KING_OFFSETS[j];
                    int target = i + offset;

                    if (target < 0 || target > 63) continue;

                    int from_file = i % 8;
                    int from_rank = i / 8;
                    int to_file   = target % 8;
                    int to_rank   = target / 8;

                    // Prevent wraparounds
                    if (abs(from_file - to_file) > 1 || abs(from_rank - to_rank) > 1) continue;

                    if (is_color(game->board[target], game->turn)) continue;

                    Move move = {
                        .from = i,
                        .to = target,
                        .promotion = false,
                        .capture = is_enemy_piece(game->board[target], game->turn),
                        .piece = W_KING
                    };

                    add_move(game, move);
                }

                // === Castling ===
                // Note: You must also check if the squares between are empty and not under attack externally
                game->can_white_castle_kingside = can_king_castle(game, WHITE, KING_SIDE);
                game->can_white_castle_queenside = can_king_castle(game, WHITE, QUEEN_SIDE);

                if (game->can_white_castle_kingside) {
                    Move move = {
                        .from = i,
                        .to = i + 2,
                        .piece = W_KING,
                        .is_castle_king_side = true
                    };
                    add_move(game, move);
                }

                if (game->can_white_castle_queenside) {
                    Move move = {
                        .from = i,
                        .to = i - 2,
                        .piece = W_KING,
                        .is_castle_queen_side = true
                    };
                    add_move(game, move);
                }

                break;
            }

            case B_PAWN:
            {
                int start_file = i % 8;
                int start_rank = i / 8;

                int target_one_step = i + PAWN_OFFSETS[2];
                int target_two_step = i + PAWN_OFFSETS[3];
                int target_capture_left = i + PAWN_OFFSETS[6];
                int target_capture_right = i + PAWN_OFFSETS[7];

                bool is_on_second_rank = (start_rank == 1);
                bool is_on_seventh_rank = (start_rank == 6); // promotion rank after move

                bool can_move_one_step = (game->board[target_one_step] == EMPTY);
                bool can_move_two_step = can_move_one_step && is_on_seventh_rank && (game->board[target_two_step] == EMPTY);

                // Normal move
                if (can_move_one_step) {
                    if (is_on_second_rank) {
                        add_move(game, (Move){.from = i, .to = target_one_step, .promotion = true, .capture = false, .piece = B_PAWN, .promotion_piece = B_QUEEN});
                        add_move(game, (Move){.from = i, .to = target_one_step, .promotion = true, .capture = false, .piece = B_PAWN, .promotion_piece = B_ROOK});
                        add_move(game, (Move){.from = i, .to = target_one_step, .promotion = true, .capture = false, .piece = B_PAWN, .promotion_piece = B_BISHOP});
                        add_move(game, (Move){.from = i, .to = target_one_step, .promotion = true, .capture = false, .piece = B_PAWN, .promotion_piece = B_KNIGHT});
                    } else {
                        add_move(game, (Move){.from = i, .to = target_one_step, .promotion = false, .capture = false, .piece = B_PAWN});
                    }
                }

                // Double push
                if (can_move_two_step) {
                    add_move(game, (Move){.from = i, .to = target_two_step, .promotion = false, .capture = false, .piece = B_PAWN});
                }

                // Captures
                if (start_file > 0 && is_enemy_piece(game->board[target_capture_right], game->turn)) {
                    if (is_on_second_rank) {
                        add_move(game, (Move){.from = i, .to = target_capture_right, .promotion = true, .capture = true, .piece = B_PAWN, .promotion_piece = B_QUEEN});
                        add_move(game, (Move){.from = i, .to = target_capture_right, .promotion = true, .capture = true, .piece = B_PAWN, .promotion_piece = B_ROOK});
                        add_move(game, (Move){.from = i, .to = target_capture_right, .promotion = true, .capture = true, .piece = B_PAWN, .promotion_piece = B_BISHOP});
                        add_move(game, (Move){.from = i, .to = target_capture_right, .promotion = true, .capture = true, .piece = B_PAWN, .promotion_piece = B_KNIGHT});
                    } else {
                        add_move(game, (Move){.from = i, .to = target_capture_right, .promotion = false, .capture = true, .piece = B_PAWN});
                    }
                }

                if (start_file < 7 && is_enemy_piece(game->board[target_capture_left], game->turn)) {
                    if (is_on_second_rank) {
                        add_move(game, (Move){.from = i, .to = target_capture_left, .promotion = true, .capture = true, .piece = B_PAWN, .promotion_piece = B_QUEEN});
                        add_move(game, (Move){.from = i, .to = target_capture_left, .promotion = true, .capture = true, .piece = B_PAWN, .promotion_piece = B_ROOK});
                        add_move(game, (Move){.from = i, .to = target_capture_left, .promotion = true, .capture = true, .piece = B_PAWN, .promotion_piece = B_BISHOP});
                        add_move(game, (Move){.from = i, .to = target_capture_left, .promotion = true, .capture = true, .piece = B_PAWN, .promotion_piece = B_KNIGHT});
                    } else {
                        add_move(game, (Move){.from = i, .to = target_capture_left, .promotion = false, .capture = true, .piece = B_PAWN});
                    }
                }

                if (game->en_passant_square != -1) {
                    int ep_square = game->en_passant_square;
                    int ep_file = ep_square % 8;
                    int ep_rank = ep_square / 8;

                    // White pawn must be on rank 5 (index 4)
                    if (start_rank == 4) {
                        // The pawn to be captured is one rank behind en passant square (rank 4)
                        int captured_pawn_square = ep_square - 8;  // one rank below ep square

                        // Check if there is actually a black pawn on the captured pawn square
                        if (game->board[captured_pawn_square] == B_PAWN) {
                            // En passant capture to the left
                            if (ep_square == target_capture_left && ep_file == start_file - 1) {
                                add_move(game, (Move){
                                    .from = i,
                                    .to = ep_square,
                                    .promotion = false,
                                    .capture = true,
                                    .piece = W_PAWN,
                                    .is_en_passant = true
                                });
                            }
                            // En passant capture to the right
                            else if (ep_square == target_capture_right && ep_file == start_file + 1) {
                                add_move(game, (Move){
                                    .from = i,
                                    .to = ep_square,
                                    .promotion = false,
                                    .capture = true,
                                    .piece = W_PAWN,
                                    .is_en_passant = true
                                });
                            }
                        }
                    }
                }


                break;
            }
            case B_KNIGHT:
            {
                for (int j = 0; j < 8; j++) {
                    int new_square = i + KNIGHT_OFFSETS[j];

                    if (new_square < 0 || new_square > 63) continue;

                    int new_file = new_square % 8;
                    int new_rank = new_square / 8;

                    // Prevent wraparound (e.g. jumping from file H to file A)
                    if (abs(new_file - file) > 2 || abs(new_rank - rank) > 2) continue;

                    if (is_color(game->board[new_square], game->turn)) continue;

                    Move move = {
                        .from = i,
                        .to = new_square,
                        .promotion = false,
                        .capture = is_enemy_piece(game->board[new_square], game->turn),
                        .piece = B_KNIGHT
                    };

                    add_move(game, move);
                }

                break;
            }
            case B_BISHOP:
            {
                for (int j = 0; j < 4; j++) { // BISHOP_OFFSETS should have 4 directions: NE, NW, SE, SW
                    int offset = BISHOP_OFFSETS[j];
                    int target = i;

                    while (true) {
                        int prev_file = target % 8;
                        int prev_rank = target / 8;

                        target += offset;

                        if (target < 0 || target > 63) break;

                        int target_file = target % 8;
                        int target_rank = target / 8;

                        // If not moving diagonally or wrapped around file boundary, stop
                        if (abs(target_file - prev_file) != 1 || abs(target_rank - prev_rank) != 1) break;

                        if (is_color(game->board[target], game->turn)) break;

                        Move move = {
                            .from = i,
                            .to = target,
                            .promotion = false,
                            .capture = is_enemy_piece(game->board[target], game->turn),
                            .piece = B_BISHOP
                        };

                        add_move(game, move);

                        if (move.capture) break;
                    }
                }

                break;
            }
            case B_ROOK:
            {
                for (int j = 0; j < 4; j++) { // ROOK_OFFSETS: +1 (→), -1 (←), +8 (↑), -8 (↓)
                    int offset = ROOK_OFFSETS[j];
                    int target = i;

                    while (true) {
                        int prev_file = target % 8;
                        int prev_rank = target / 8;

                        target += offset;

                        if (target < 0 || target > 63) break;

                        int target_file = target % 8;
                        int target_rank = target / 8;

                        // Prevent wrapping in horizontal directions
                        if ((offset == 1 || offset == -1) && target_rank != prev_rank) break;

                        if (is_color(game->board[target], game->turn)) break;

                        Move move = {
                            .from = i,
                            .to = target,
                            .promotion = false,
                            .capture = is_enemy_piece(game->board[target], game->turn),
                            .piece = B_ROOK
                        };

                        add_move(game, move);

                        if (move.capture) break;
                    }
                }

                break;
            }
            case B_QUEEN:
            {
                for (int j = 0; j < 8; j++) { // QUEEN_OFFSETS: combines ROOK and BISHOP directions
                    int offset = QUEEN_OFFSETS[j];
                    int target = i;

                    while (true) {
                        int prev_file = target % 8;
                        int prev_rank = target / 8;

                        target += offset;
                        if (target < 0 || target > 63) break;

                        int target_file = target % 8;
                        int target_rank = target / 8;

                        // Prevent file/rank wraparounds
                        if ((offset == 1 || offset == -1) && target_rank != prev_rank) break;
                        if ((offset == 8 || offset == -8) && target_file != prev_file) break;
                        if ((offset == 7 || offset == -7 || offset == 9 || offset == -9) &&
                            abs(target_file - prev_file) != 1) break;

                        if (is_color(game->board[target], game->turn)) break;

                        Move move = {
                            .from = i,
                            .to = target,
                            .promotion = false,
                            .capture = is_enemy_piece(game->board[target], game->turn),
                            .piece = B_QUEEN
                        };

                        add_move(game, move);

                        if (move.capture) break;
                    }
                }

                break;
            }
            case B_KING:
            {
                for (int j = 0; j < 8; j++) {
                    int offset = KING_OFFSETS[j];
                    int target = i + offset;

                    if (target < 0 || target > 63) continue;

                    int from_file = i % 8;
                    int from_rank = i / 8;
                    int to_file   = target % 8;
                    int to_rank   = target / 8;

                    // Prevent wraparounds
                    if (abs(from_file - to_file) > 1 || abs(from_rank - to_rank) > 1) continue;

                    if (is_color(game->board[target], game->turn)) continue;

                    Move move = {
                        .from = i,
                        .to = target,
                        .promotion = false,
                        .capture = is_enemy_piece(game->board[target], game->turn),
                        .piece = B_KING
                    };

                    add_move(game, move);
                }

                // === Castling ===
                // Note: You must also check if the squares between are empty and not under attack externally
                game->can_black_castle_kingside = can_king_castle(game, BLACK, KING_SIDE);
                game->can_black_castle_queenside = can_king_castle(game, BLACK, QUEEN_SIDE);

                if (game->can_white_castle_kingside) {
                    Move move = {
                        .from = i,
                        .to = i + 2,
                        .piece = B_KING,
                        .is_castle_king_side = true
                    };
                    add_move(game, move);
                }

                if (game->can_white_castle_queenside) {
                    Move move = {
                        .from = i,
                        .to = i - 2,
                        .piece = B_KING,
                        .is_castle_queen_side = true
                    };
                    add_move(game, move);
                }

                break;
            }
            default:
                // Handle empty or invalid piece
                break;
        }
    }
}

int score_move(GameState *game, Move m)
{
    int attacker = game->board[m.from];
    int victim = game->board[m.to];

    if (victim != EMPTY)
    {
        return (10 * piece_value(victim)) - piece_value(attacker);
    }

    if (m.promotion) return 10000 + m.promotion_piece;
    if (m.is_castle_king_side) return 5000;
    if (m.is_castle_queen_side) return 5000;
}

void sort_moves(GameState *game)
{
    if (!game || game->move_count == 0) return;

    int scores[MAX_LEGAL_MOVES];

    // Precompute all scores once
    for (int i = 0; i < game->move_count; i++) {
        scores[i] = score_move(game, game->movelist[i]);
    }

    // Insertion sort
    for (int i = 1; i < game->move_count; i++) {
        Move key_move = game->movelist[i];
        int key_score = scores[i];
        int j = i - 1;

        while (j >= 0 && scores[j] < key_score) {
            game->movelist[j + 1] = game->movelist[j];
            scores[j + 1] = scores[j];
            j--;
        }

        game->movelist[j + 1] = key_move;
        scores[j + 1] = key_score;
    }
}