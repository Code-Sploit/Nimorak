#include "nimorak.h"
#include "board.h"
#include "constants.h"
#include "evaluation.h"
#include "search.h"
#include "movegen.h"
#include "helper.h"
#include "perft.h"
#include "attack.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

void uci_loop(GameState *game)
{
    if (!game) return;

    char line[512];

    while (1)
    {
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) continue;

        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "ucinewgame", 10) == 0)
        {
            clear_board(game);
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
                load_fen(game, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                ptr += 8;

                if (strncmp(ptr, " moves", 6) == 0)
                {
                    ptr += 6;

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

                        make_move_str(game, move_str);
                    }
                }
            }

            generate_attack_tables(game, WHITE);
            generate_attack_tables(game, BLACK);
        }
        else if (strncmp(line, "go", 2) == 0)
        {
            generate_legal_moves(game);

            int depth = 5;

            clock_t start_time = clock();

            Move best_move = find_best_move(game, depth);
            
            clock_t end_time = clock();

            double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;

            printf("info depth %d eval %f\n", depth, best_move.eval);
            printf("bestmove %s%s%c\n", translate_square_to_string(best_move.from), translate_square_to_string(best_move.to), get_promotion_piece(best_move));
            printf("info time %.3f seconds\n", time_spent);

            fflush(stdout);
        }
        else if (strncmp(line, "perft", 5) == 0)
        {
            perft_print_breakdown(game, 1);
            perft_print_breakdown(game, 2);
            perft_print_breakdown(game, 3);
            perft_print_breakdown(game, 4);
            perft_print_breakdown(game, 5);
            perft_print_breakdown(game, 6);
        }
        else if (strncmp(line, "atw", 3) == 0)
        {
            print_attack_tables(game, WHITE);
        }
        else if (strncmp(line, "atb", 3) == 0)
        {
            print_attack_tables(game, BLACK);
        }
        else if (strncmp(line, "d", 1) == 0)
        {
            print_board(game);
        }
    }
}

int main(int argc, char** argv)
{
    GameState *game = new_game(WHITE, "2rr4/7p/8/3p1k2/pb3P2/7P/8/5RK1 w - - 1 34");

    //for (int rank = 7; rank >= 0; rank--) {
      //  for (int file = 0; file < 8; file++) {
        //    int sq = rank * 8 + file;
          //  printf("%2d ", game->board[sq]);
        //}
        //printf("\n");
//    }

    uci_loop(game);

    return 0;
}