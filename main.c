#include "nimorak.h"
#include "board.h"
#include "constants.h"
#include "search.h"
#include "movegen.h"
#include "helper.h"
#include "perft.h"
#include "attack.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define POSITION_STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

int get_random(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void uci_loop(GameState *game)
{
    if (!game) return;

    char line[4096];

    while (1)
    {
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) continue;

        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "ucinewgame", 10) == 0)
        {
            board_clear(game);
        }
        else if (strcmp(line, "uci") == 0)
        {
            printf("id name Nimorak\n");
            printf("id author Samuel 't Hart\n");
            printf("uciok\n");

            fflush(stdout);
        }
        else if (strcmp(line, "isready") == 0)
        {
            printf("readyok\n");

            fflush(stdout);
        }
        else if (strncmp(line, "position", 8) == 0)
        {
            char *ptr = line + 9;

            if (strncmp(ptr, "startpos", 8) == 0)
            {
                board_load_fen(game, POSITION_STARTPOS_FEN);
                ptr += 8;
            }
            else if (strncmp(ptr, "fen", 3) == 0)
            {
                ptr += 3;

                while (*ptr && isspace(*ptr)) ptr++;

                char fen[128] = {0};
                int i = 0;
                int spaces = 0;

                // FEN has exactly 6 space-separated fields
                while (*ptr && spaces < 6 && i < (int)(sizeof(fen) - 1))
                {
                    if (isspace(*ptr))
                        spaces++;

                    fen[i++] = *ptr++;
                }

                fen[i] = '\0';
                board_load_fen(game, fen);
            }

            // Now parse optional moves
            while (*ptr && isspace(*ptr)) ptr++;

            if (strncmp(ptr, "moves", 5) == 0)
            {
                ptr += 5;

                while (*ptr)
                {
                    while (*ptr && isspace(*ptr)) ptr++;

                    if (!*ptr) break;

                    char move_str[6] = {0};
                    int j = 0;

                    while (*ptr && !isspace(*ptr) && j < 5)
                    {
                        move_str[j++] = *ptr++;
                    }

                    move_str[j] = '\0';

                    board_make_move_str(game, move_str);
                }
            }

            attack_generate_table(game, WHITE);
            attack_generate_table(game, BLACK);
        }
        else if (strncmp(line, "go", 2) == 0)
        {
            movegen_generate_legal_moves(game);

            int index = get_random(0, game->move_count);

            Move move = game->movelist[get_random(0, game->move_count - 1)];

            char *promotion_string = (move.promotion) ? piece_to_char(move.promotion_piece) : " ";

            printf("bestmove %s%s%s\n", translate_square_to_string(move.from), translate_square_to_string(move.to), promotion_string);

            fflush(stdout);
        }
        else if (strncmp(line, "perft_tp", 8) == 0)
        {
            perft_run_test_positions(game);
        }
        else if (strncmp(line, "perft", 5) == 0)
        {
            int depth = 1;
            char *ptr = line + 5;

            // Skip whitespace after "perft"
            while (*ptr && isspace(*ptr)) ptr++;

            // If there's a number, parse it
            if (*ptr)
                depth = atoi(ptr);

            perft_divide(game, depth, true);
        }
        else if (strncmp(line, "atw", 3) == 0)
        {
            attack_print_table(game, WHITE);
        }
        else if (strncmp(line, "atb", 3) == 0)
        {
            attack_print_table(game, BLACK);
        }
        else if (strncmp(line, "d", 1) == 0)
        {
            board_print(game);
        }
    }
}

int main(int argc, char** argv)
{
    GameState *game = game_new();

    nimorak_startup();

    uci_loop(game);

    return 0;
}