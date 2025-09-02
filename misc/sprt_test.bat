@echo off
set start=%time%

.\misc\cutechess-cli.exe -tournament gauntlet -pgnout results.pgn -concurrency 4 ^
-openings file=".\misc\silversuite.pgn" format=pgn order=random plies=8 ^
-engine cmd=.\%1.exe name=%1 proto=uci st=0.075 ^
-engine cmd=.\%2.exe name=%2 proto=uci st=0.075 ^
-repeat -games %3

set end=%time%
echo Start: %start%
echo End:   %end%
