echo Compiling Nimorak chess engine...

FLAGS="-Ofast -march=native -std=c99 -Wall -Wextra -flto -DNDEBUG -funroll-loops -fomit-frame-pointer -fstrict-aliasing"

INCLUDE="-Iinclude"

SOURCES="src/board/attack.c src/board/board.c src/search/eval.c src/main.c src/board/movegen.c src/nimorak/nimorak.c src/search/perft.c src/table/table.c src/search/search.c src/table/repetition.c src/table/transposition.c src/table/zobrist.c"

OUTPUT="-o nimorak"

gcc $FLAGS $INCLUDE $SOURCES $OUTPUT -lm
