#include <storage/transposition.hpp>
#include <storage/repetition.hpp>
#include <tables/zobrist.hpp>
#include <tables/magic.hpp>
#include <tables/table.hpp>

#include <core/movegen.hpp>
#include <core/attack.hpp>
#include <core/board.hpp>
#include <core/search.hpp>
#include <core/perft.hpp>
#include <core/eval.hpp>

#include <tables/helpers.hpp>

#include <utils/config.hpp>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <cmath>
#include <cstdlib>

void uciLoop(Nimorak::Game& game)
{
    char input[4096];

    while (fgets(input, sizeof(input), stdin))
    {
        // Strip newline
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
            game.historyCount = 0;
            game.zobristKey = 0ULL;
            game.isFirstLoad = 1;

            game.repetitionTable.clear();
            game.transpositionTable.clear();
        }
        else if (strcmp(input, "isready") == 0)
        {
            printf("readyok\n");
            fflush(stdout);
        }
        else if (strncmp(input, "setoption", 9) == 0)
        {
            char name[128], value[128];
            if (sscanf(input, "setoption name %127s value %127s", name, value) == 2)
            {
                if (Config::setOption(game, name, value))
                {
                    printf("info string unknown option: %s\n", name);
                }
            }
            else
            {
                printf("info string invalid setoption command\n");
            }
            fflush(stdout);
        }
        else if (strncmp(input, "position", 8) == 0)
        {
            char *ptr = input + 9;

            if (strncmp(ptr, "startpos", 8) == 0)
            {
                Board::loadFen(game, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                ptr += 8;
            }
            else if (strncmp(ptr, "fen", 3) == 0)
            {
                ptr += 4;
                char fen[4096];
                strcpy(fen, ptr);

                char *moves_pos = strstr(fen, " moves");
                if (moves_pos) *moves_pos = '\0';

                Board::loadFen(game, fen);
            }

            // Parse moves after "moves"
            char *moves = strstr(ptr, "moves");
            if (moves)
            {
                moves += 6;
                while (*moves)
                {
                    char move_str[6];
                    if (sscanf(moves, "%5s", move_str) != 1) break;

                    Move move = Board::parseMove(game, move_str);
                    Board::makeMove(game, move, MAKE_MOVE_FULL);

                    moves += strcspn(moves, " ");
                    while (*moves == ' ') moves++;
                }
            }

            game.attackWorker.generateAll(game);

            Zobrist::updateBoard(game);

            fflush(stdout);
        }
        else if (strncmp(input, "go perft ", 9) == 0)
        {
            Perft::root(game, std::atoi(input + 9));
        }
        else if (strncmp(input, "go perfttest", 12) == 0)
        {
            Perft::runTests(game);
        }
        else if (strncmp(input, "go", 2) == 0)
        {
            char *ptr = input + 2;
            int wtime = -1, btime = -1, winc = 0, binc = 0;
            int movestogo = 30, depth = -1, movetime = -1;

            while (*ptr)
            {
                if (strncmp(ptr, "wtime", 5) == 0) { ptr += 5; while (*ptr == ' ') ptr++; wtime = atoi(ptr); }
                else if (strncmp(ptr, "btime", 5) == 0) { ptr += 5; while (*ptr == ' ') ptr++; btime = atoi(ptr); }
                else if (strncmp(ptr, "winc", 4) == 0)  { ptr += 4; while (*ptr == ' ') ptr++; winc = atoi(ptr); }
                else if (strncmp(ptr, "binc", 4) == 0)  { ptr += 4; while (*ptr == ' ') ptr++; binc = atoi(ptr); }
                else if (strncmp(ptr, "movestogo", 9) == 0) { ptr += 9; while (*ptr == ' ') ptr++; movestogo = atoi(ptr); }
                else if (strncmp(ptr, "depth", 5) == 0) { ptr += 5; while (*ptr == ' ') ptr++; depth = atoi(ptr); }
                else if (strncmp(ptr, "movetime", 8) == 0) { ptr += 8; while (*ptr == ' ') ptr++; movetime = atoi(ptr); }

                while (*ptr && *ptr != ' ') ptr++;
                while (*ptr == ' ') ptr++;
            }

            Move best_move;

            if (movetime > 0)
                best_move = game.searchWorker.searchPosition(game, game.config.search.maximumDepth, movetime);
            else if (depth > 0)
                best_move = game.searchWorker.searchPosition(game, std::min(game.config.search.maximumDepth, depth), 1000000);
            else if (wtime > 0 && btime > 0)
            {
                int time_left = (game.turn == WHITE) ? wtime : btime;
                int increment = (game.turn == WHITE) ? winc : binc;
                if (movestogo <= 0) movestogo = 30;

                int base_time = time_left / movestogo;
                int think_time = base_time + increment / 2;

                int max_time = time_left * 60 / 100;
                if (think_time > max_time) think_time = max_time;
                if (think_time < 10) think_time = 10;
                if (time_left < 60000) { think_time = time_left / 10; if (think_time < 5) think_time = 5; }

                best_move = game.searchWorker.searchPosition(game, 64, think_time);
            }
            else
            {
                best_move = game.searchWorker.searchPosition(game, std::min(game.config.search.initialDepth, game.config.search.maximumDepth), 1000000);
            }

            printf("bestmove %s\n", (best_move == 0) ? "(none)" : Board::moveToString(best_move).c_str());
            fflush(stdout);
        }
        else if (strcmp(input, "config") == 0)
        {
            printf("=== Current Configuration ===\n");

            // MoveGen options
            printf("MoveGen:\n");
            printf("  doLegalMoveFiltering: %d\n", game.config.moveGen.doLegalMoveFiltering);
            printf("  doOnlyCaptures:       %d\n", game.config.moveGen.doOnlyCaptures);

            // Eval options
            printf("Eval:\n");
            printf("  doMaterial:           %d\n", game.config.eval.doMaterial);
            printf("  doPieceSquares:       %d\n", game.config.eval.doPieceSquares);
            printf("  doEndgame:            %d\n", game.config.eval.doEndgame);
            printf("  doMobility:           %d\n", game.config.eval.doMobility);
            printf("  doKingSafety:         %d\n", game.config.eval.doKingSafety);
            printf("  doPawnStructure:      %d\n", game.config.eval.doPawnStructure);
            printf("  doRookOpenFiles:      %d\n", game.config.eval.doRookOpenFiles);
            printf("  doBishopPair:         %d\n", game.config.eval.doBishopPair);
            printf("  doKnightOutposts:     %d\n", game.config.eval.doKnightOutposts);

            // Search options
            printf("Search:\n");
            printf("  doQuiescense:         %d\n", game.config.search.doQuiescense);
            printf("  doTranspositions:     %d\n", game.config.search.doTranspositions);
            printf("  doKillerMoves:        %d\n", game.config.search.doKillerMoves);
            printf("  doHeuristics:         %d\n", game.config.search.doHeuristics);
            printf("  doInfo:               %d\n", game.config.search.doInfo);
            printf("  initialDepth:         %d\n", game.config.search.initialDepth);
            printf("  maximumDepth:         %d\n", game.config.search.maximumDepth);
            printf("  maximumQuiescenseDepth:%d\n", game.config.search.maximumQuiescenseDepth);

            printf("===========================\n");
            fflush(stdout);
        }
        else if (strcmp(input, "quit") == 0 || strcmp(input, "stop") == 0)
        {
            break;
        }
        else if (strcmp(input, "display") == 0)
        {
            Board::print(game);
            fflush(stdout);
        }
        else if (strcmp(input, "atw") == 0) game.attackWorker.printTable(game, WHITE);
        else if (strcmp(input, "atb") == 0) game.attackWorker.printTable(game, BLACK);
        else if (strcmp(input, "eval") == 0) printf("Eval: %d\n", game.evalWorker.evaluate(game));
    }
}

int main()
{
    srand(1234);

    Nimorak::Game game;

    uciLoop(game);

    return 0;
}
