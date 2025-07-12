@echo off
setlocal

REM Check if a version parameter is passed
if "%~1"=="" (
    set "VERSION=nimorak"
) else (
    set "VERSION=nimorak-%~1"
)

echo [*] Building %VERSION%...

REM Set output file name
set "OUTPUT=%VERSION%.exe"

REM Clean previous build
if exist "%OUTPUT%" del "%OUTPUT%"

REM Compile all source files
gcc main.c nimorak.c board.c movegen.c attack.c helper.c perft.c -o "%OUTPUT%" -O2 -std=c99

REM Check if build succeeded
if exist "%OUTPUT%" (
    echo [✓] Build successful: %OUTPUT%
) else (
    echo [X] Build failed.
)

endlocal
pause
