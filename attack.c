#include "nimorak.h"
#include "attack.h"
#include "helper.h"

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define IS_ON_BOARD(sq) ((sq) >= 0 && (sq) < 64)
#define WRAPPED_FILE(prev, curr) (abs((curr % 8) - (prev % 8)) != 1)

void attack_clear_table(GameState *game, int color)
{
    if (!game) return;

    memset(color == WHITE ? game->attack_table_white : game->attack_table_black, 0, sizeof(int) * 64);
}

void attack_generate_table(GameState *game, int color) {
    if (!game) return;

    attack_clear_table(game, color);

    int *table = (color == WHITE) ? game->attack_table_white : game->attack_table_black;

    for (int i = 0; i < 64; i++) {
        int piece = game->board[i];
        
        if (!is_same_color(piece, color)) continue;

        int file = i % 8;
        int rank = i / 8;

        switch (piece) {
            case W_PAWN:
            case B_PAWN: {
                int dir[2] = { (color == WHITE ? 7 : -9), (color == WHITE ? 9 : -7) };

                for (int j = 0; j < 2; j++) {
                    int target = i + dir[j];
                    
                    if (!IS_ON_BOARD(target)) continue;

                    int target_file = target % 8;
                    
                    if (abs(target_file - file) == 1)
                        table[target] = 1;
                }
                break;
            }

            case W_KNIGHT:
            case B_KNIGHT: {
                for (int j = 0; j < 8; j++) {
                    int target = i + KNIGHT_OFFSETS[j];
                    
                    if (!IS_ON_BOARD(target)) continue;

                    int d_file = abs((target % 8) - file);
                    int d_rank = abs((target / 8) - rank);
                    
                    if (d_file <= 2 && d_rank <= 2)
                        table[target] = 1;
                }
                break;
            }

            case W_BISHOP:
            case B_BISHOP: {
                for (int j = 0; j < 4; j++) {
                    int offset = BISHOP_OFFSETS[j];
                    int target = i;

                    while (true) {
                        int prev_file = target % 8;
                        
                        target += offset;
                        
                        if (!IS_ON_BOARD(target)) break;
                        if (WRAPPED_FILE(prev_file, target)) break;

                        table[target] = 1;
                        
                        if (game->board[target] != EMPTY) break;
                    }
                }
                break;
            }

            case W_ROOK:
            case B_ROOK: {
                for (int j = 0; j < 4; j++) {
                    int offset = ROOK_OFFSETS[j];
                    int target = i;

                    while (true) {
                        int prev_file = target % 8;
                        
                        target += offset;
                        
                        if (!IS_ON_BOARD(target)) break;
                        if ((offset == 1 || offset == -1) && WRAPPED_FILE(prev_file, target)) break;

                        table[target] = 1;
                        
                        if (game->board[target] != EMPTY) break;
                    }
                }
                break;
            }

            case W_QUEEN:
            case B_QUEEN: {
                for (int j = 0; j < 8; j++) {
                    int offset = QUEEN_OFFSETS[j];
                    int target = i;

                    while (true) {
                        int prev_file = target % 8;

                        target += offset;
                        
                        if (!IS_ON_BOARD(target)) break;
                        if ((offset == 1 || offset == -1 || offset == 9 || offset == -9 || offset == 7 || offset == -7) &&
                            WRAPPED_FILE(prev_file, target)) break;

                        table[target] = 1;
                        
                        if (game->board[target] != EMPTY) break;
                    }
                }
                break;
            }

            case W_KING:
            case B_KING: {
                for (int j = 0; j < 8; j++) {
                    int target = i + KING_OFFSETS[j];
                    
                    if (!IS_ON_BOARD(target)) continue;

                    int d_file = abs((target % 8) - file);
                    int d_rank = abs((target / 8) - rank);
                    
                    if (d_file <= 1 && d_rank <= 1)
                        table[target] = 1;
                }
                break;
            }
        }
    }
}

void attack_print_table(GameState *game, int color)
{
    const int *table = (color == WHITE) ? game->attack_table_white : game->attack_table_black;
    const char* color_name = (color == WHITE) ? "White" : "Black";

    printf("%s Attack Table:\n", color_name);
    printf("  +-----------------+\n");
    
    for (int rank = 7; rank >= 0; rank--)
    {
        printf("%d | ", rank + 1);

        for (int file = 0; file < 8; file++)
        {
            printf("%d ", table[rank * 8 + file]);
        }

        printf("|\n");
    }

    printf("  +-----------------+\n");
    printf("    a b c d e f g h\n");
}
