#include <table/transposition.h>
#include <table/repetition.h>
#include <table/zobrist.h>
#include <table/magic.h>
#include <table/table.h>

#include <board/movegen.h>
#include <board/attack.h>
#include <board/board.h>

#include <search/search.h>
#include <search/perft.h>
#include <search/eval.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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
        else if (strcmp(input, "ucinewgame") == 0)
        {
            game->history_count = 0;
            game->zobrist_key = 0ULL;
            game->board_is_first_load = 0;
            
            repetition_clear(game);
            tt_clear(game);
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
            char *ptr = input + 2;

            int wtime = -1, btime = -1;
            int winc = 0, binc = 0;
            int movestogo = 30;
            int depth = -1;
            int movetime = -1;

            while (*ptr)
            {
                if (strncmp(ptr, "wtime", 5) == 0)
                {
                    ptr += 5; while (*ptr == ' ') ptr++;
                    wtime = atoi(ptr);
                }
                else if (strncmp(ptr, "btime", 5) == 0)
                {
                    ptr += 5; while (*ptr == ' ') ptr++;
                    btime = atoi(ptr);
                }
                else if (strncmp(ptr, "winc", 4) == 0)
                {
                    ptr += 4; while (*ptr == ' ') ptr++;
                    winc = atoi(ptr);
                }
                else if (strncmp(ptr, "binc", 4) == 0)
                {
                    ptr += 4; while (*ptr == ' ') ptr++;
                    binc = atoi(ptr);
                }
                else if (strncmp(ptr, "movestogo", 9) == 0)
                {
                    ptr += 9; while (*ptr == ' ') ptr++;
                    movestogo = atoi(ptr);
                }
                else if (strncmp(ptr, "depth", 5) == 0)
                {
                    ptr += 5; while (*ptr == ' ') ptr++;
                    depth = atoi(ptr);
                }
                else if (strncmp(ptr, "movetime", 8) == 0)
                {
                    ptr += 8; while (*ptr == ' ') ptr++;
                    movetime = atoi(ptr);
                }

                while (*ptr && *ptr != ' ')
                    ptr++;
                while (*ptr == ' ')
                    ptr++;
            }

            Move best_move;

            if (movetime > 0)
            {
                best_move = search_start(game, 64, movetime);
            }
            else if (depth > 0)
            {
                best_move = search_start(game, depth, INFINITE_TIME);
            }
            else if (wtime > 0 && btime > 0)
            {
                int time_left = (game->turn == WHITE) ? wtime : btime;
                int increment = (game->turn == WHITE) ? winc : binc;

                if (movestogo <= 0) movestogo = 30;

                // Base think time
                int base_time = time_left / movestogo;
                int think_time = base_time + increment / 2;

                // Cap to 60% of remaining time
                int max_time = time_left * 60 / 100;
                if (think_time > max_time)
                    think_time = max_time;

                // Minimum think time
                if (think_time < 10)
                    think_time = 10;

                // Critical low time scaling (<1 min)
                if (time_left < 60000) // less than 1 minute
                {
                    think_time = time_left / 10; // only 10% of remaining time
                    if (think_time < 5) think_time = 5; // at least 5ms
                }

                best_move = search_start(game, 64, think_time);
            }
            else
            {
                best_move = search_start(game, SEARCH_INITIAL_DEPTH, INFINITE_TIME);
            }

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
            printf("Eval: %d\n", eval_position(game));
        }
    }
}

int main()
{
    srand(1234);

    Game *game = game_new();

    table_precompute_all_attacks(game);

    zobrist_init();

    uci_loop(game);

    return 0;
}
