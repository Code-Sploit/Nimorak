@echo off
echo Compiling Nimorak chess engine...

REM Set compiler flags
set FLAGS=-Ofast -march=native -std=c99 -Wall -Wextra -flto -g

REM Source files
set SOURCES=attack.c board.c magic.c main.c movegen.c nimorak.c perft.c table.c

REM Output file
set OUTPUT=-o nimorak.exe

REM Compile
gcc %FLAGS% %SOURCES% %OUTPUT%

if %errorlevel% neq 0 (
    echo ❌ Build failed.
    exit /b %errorlevel%
) else (
    echo ✅ Build succeeded. Output: nimorak.exe
)
