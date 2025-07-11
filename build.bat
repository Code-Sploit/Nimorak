@echo off
setlocal

echo [*] Building Nimorak...

REM Set output file name
set OUTPUT=nimorak.exe

REM Clean previous build
if exist %OUTPUT% del %OUTPUT%

REM Compile all source files
gcc main.c nimorak.c board.c movegen.c evaluation.c attack.c helper.c perft.c search.c -o %OUTPUT% -O2 -std=c99

REM Check if build succeeded
if exist %OUTPUT% (
    echo [✓] Build successful: %OUTPUT%
) else (
    echo [X] Build failed.
)

endlocal
pause