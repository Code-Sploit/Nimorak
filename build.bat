@echo off
echo Compiling Nimorak chess engine...

REM Set compiler flags for optimization, debugging, and warnings
set FLAGS=-Ofast -march=native -std=c99 -Wall -Wextra -flto -g -DNDEBUG -funroll-loops -fomit-frame-pointer

REM Include directory
set INCLUDE=-Iinclude

REM Source files with relative paths
set SOURCES=src/board/attack.c src/board/board.c src/search/eval.c src/main.c src/board/movegen.c src/nimorak/nimorak.c src/search/perft.c src/table/table.c src/search/search.c src/table/repetition.c src/table/transposition.c src/table/zobrist.c

REM Output file
set OUTPUT=-o nimorak.exe

REM Compile
gcc %FLAGS% %INCLUDE% %SOURCES% %OUTPUT%

if %errorlevel% neq 0 (
    echo ❌ Build failed.
    exit /b %errorlevel%
) else (
    echo ✅ Build succeeded. Output: nimorak.exe
)
