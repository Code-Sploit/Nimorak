cmake_minimum_required(VERSION 3.16)
project(Nimorak)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -march=native -Wall -Wextra -flto -DNDEBUG -funroll-loops -fomit-frame-pointer -fstrict-aliasing")

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/board/attack.c
    src/board/board.c
    src/search/eval.c
    src/main.c
    src/board/movegen.c
    src/nimorak/nimorak.c
    src/search/perft.c
    src/table/table.c
    src/search/search.c
    src/table/repetition.c
    src/table/transposition.c
    src/table/zobrist.c
)

# Create executable
add_executable(nimorak ${SOURCES})

# Link math library
target_link_libraries(nimorak m)
