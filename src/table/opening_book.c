#include <table/opening_book.h>

#include <board/board.h>

void opening_book_load(Game *game)
{
    FILE *file = fopen("book.txt", "r"); // Open file for reading

    if (!file) {
        perror("Error opening file");
        return 1;
    }

    char line[1024]; // Buffer to hold each line
    
    while (fgets(line, sizeof(line), file)) {
        // line now contains the current line, including the newline character
        char *token = strtok(moves, " "); // Split by spaces

        while (token != NULL)
        {
            Move converted_move = board_parse_move(game, token);

            token = strtok(NULL, " ");   // Get the next token
        }
    }

    fclose(file); // Always close the file
    
    return 0;
}

Move opening_book_next_move(Game *game);