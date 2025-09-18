#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_GAMES 5000
#define MAX_LINE 10000

int main() {
    FILE *fin = fopen("games.uci", "r");
    FILE *fout = fopen("opening_book.h", "w");
    if (!fin || !fout) {
        perror("File open failed");
        return 1;
    }

    char line[MAX_LINE];
    char *games[MAX_GAMES];
    int game_count = 0;

    while (fgets(line, sizeof(line), fin)) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;

        // Skip empty lines
        if (strlen(line) == 0 || strcmp(line, "---") == 0) continue;

        // Check for duplicates
        int duplicate = 0;
        for (int i = 0; i < game_count; i++) {
            if (strcmp(line, games[i]) == 0) {
                duplicate = 1;
                break;
            }
        }
        if (duplicate) continue;

        // Store the game
        games[game_count] = strdup(line);
        if (!games[game_count]) {
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }
        game_count++;
        if (game_count >= MAX_GAMES) break;
    }

    // Write header
    fprintf(fout, "#ifndef OPENING_BOOK_H\n#define OPENING_BOOK_H\n\n");
    fprintf(fout, "const int opening_book_size = %d;\n", game_count);
    fprintf(fout, "const char *opening_book[] = {\n");

    for (int i = 0; i < game_count; i++) {
        fprintf(fout, "    \"%s\"%s\n", games[i], (i == game_count - 1) ? "" : ",");
        free(games[i]);
    }

    fprintf(fout, "};\n\n#endif // OPENING_BOOK_H\n");

    fclose(fin);
    fclose(fout);

    printf("Header generated with %d unique games.\n", game_count);
    return 0;
}
