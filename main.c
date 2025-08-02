#include "board.h"
#include "movegen.h"
#include "perft.h"
#include "attack.h"
#include "table.h"
#include "magic.h"
#include "eval.h"
#include "search.h"

#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

void uci_loop(Game *game)
{
    char input[4096];

    while (fgets(input, sizeof(input), stdin))
    {
        // Remove newline characters
        input[strcspn(input, "\r\n")] = 0;

        if (strcmp(input, "uci") == 0)
        {
            printf("id name Nimorak\n");
            printf("id author Samuel 't Hart\n");
            printf("uciok\n");

            fflush(stdout);
        }
        else if (strcmp(input, "isready") == 0)
        {
            printf("readyok\n");

            fflush(stdout);
        }
        else if (strncmp(input, "position", 8) == 0)
        {
            char *ptr = input + 9;

            if (strncmp(ptr, "startpos", 8) == 0)
            {
                board_load_fen(game, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                ptr += 8;
            }
            else if (strncmp(ptr, "fen", 3) == 0)
            {
                ptr += 4;
                char fen[4096];
                strcpy(fen, ptr);
                
                // Stop at "moves" if present
                char *moves_pos = strstr(fen, " moves");
                if (moves_pos)
                {
                    *moves_pos = '\0';
                }
                
                board_load_fen(game, fen);
            }

            // Handle moves
            char *moves = strstr(ptr, "moves");
            if (moves)
            {
                moves += 6; // skip "moves "
                while (*moves)
                {
                    char move_str[6];
                    sscanf(moves, "%s", move_str);

                    Move move = board_parse_move(game, move_str);

                    board_make_move(game, move);

                    // Advance to next move
                    moves += strcspn(moves, " ");
                    while (*moves == ' ') moves++;
                }
            }

            attack_print_table(game, WHITE);

            fflush(stdout);
        }
        else if (strncmp(input, "go perft ", 9) == 0)
        {
            int depth = atoi(input + 9);
            perft_root(game, depth);
        }
        else if (strncmp(input, "go perfttest", 12) == 0)
        {
            perft_run_tests(game);
        }
        else if (strncmp(input, "go", 2) == 0)
        {
            Move best_move = search_start(game, SEARCH_INITIAL_DEPTH);

            printf("bestmove %s\n", board_move_to_string(best_move));

            fflush(stdout);
        }
        else if (strcmp(input, "quit") == 0)
        {
            break;
        }
        else if (strcmp(input, "display") == 0)
        {
            board_print(game);

            fflush(stdout);
        }
        else if (strcmp(input, "atw") == 0)
        {
            attack_print_table(game, WHITE);
        }
        else if (strcmp(input, "atb") == 0)
        {
            attack_print_table(game, BLACK);
        }
        else if (strcmp(input, "eval") == 0)
        {
            eval_position(game);
        }
    }
}

int main()
{
    srand(1234);

    Game *game = game_new();

    table_precompute_all_attacks(game);

    uci_loop(game);

    return 0;
}
