import re
import chess
import chess.engine

# --- CONFIG ---
INPUT_HEADER = r"C:\Users\thart\chess_programming\Nimorak\include\storage\openingbook.hpp"
OUTPUT_HEADER = "filtered_opening_book.hpp"
STOCKFISH_PATH = r"C:\Users\thart\Downloads\stockfish-windows-x86-64-avx2\stockfish\stockfish-windows-x86-64-avx2.exe"
MAX_MOVES_TO_EVAL = 12  # Max plies to check
BEST_N = 300
ENGINE_DEPTH = 12
MIN_SCORE = -50  # Minimum evaluation in centipawns to consider a line "good" (negative = Black advantage)

# --- Step 1: Read header file ---
with open(INPUT_HEADER, "r", encoding="utf-8") as f:
    content = f.read()

# --- Step 2: Extract valid openings ---
openings = []
for match in re.finditer(r'\{([^\}]+)\}', content):
    line = match.group(1)
    moves = [m.strip().strip('"') for m in line.split(",")]
    valid_moves = [m for m in moves if re.match(r'^[a-h][1-8][a-h][1-8][qrbn]?$', m)]
    if valid_moves:
        openings.append(valid_moves)

print(f"Found {len(openings)} valid openings.")

# --- Step 3: Evaluate with Stockfish considering opponent responses ---
engine = chess.engine.SimpleEngine.popen_uci(STOCKFISH_PATH)
evaluated_openings = []

for idx, moves in enumerate(openings):
    board = chess.Board()
    valid = True
    for ply, move_uci in enumerate(moves[:MAX_MOVES_TO_EVAL]):
        move = chess.Move.from_uci(move_uci)
        if move not in board.legal_moves:
            print(f"Skipping illegal move {move_uci} in opening {idx}")
            valid = False
            break
        board.push(move)
        
        # Evaluate the position after each ply
        info = engine.analyse(board, chess.engine.Limit(depth=ENGINE_DEPTH))
        score = info["score"].white().score(mate_score=100000)
        if score is None:
            score = 0
        # If evaluation drops below MIN_SCORE for White, discard this line
        if score < MIN_SCORE:
            valid = False
            break

    if valid:
        # Use the score after the last evaluated move
        evaluated_openings.append((score, moves))

engine.quit()

# --- Step 4: Keep best N openings ---
evaluated_openings.sort(key=lambda x: x[0], reverse=True)  # Higher = better for White
best_openings = [moves for score, moves in evaluated_openings[:BEST_N]]

print(f"Keeping {len(best_openings)} best openings.")

# --- Step 5: Write filtered opening book ---
with open(OUTPUT_HEADER, "w", encoding="utf-8") as f:
    f.write("#pragma once\n\n")
    f.write("#include <vector>\n#include <string>\n\n")
    f.write("namespace OpeningBookData {\n")
    f.write("    const std::vector<std::vector<std::string>> openingBook = {\n")
    for moves in best_openings:
        f.write("        {")
        f.write(", ".join(f'"{m}"' for m in moves))
        f.write("},\n")
    f.write("    };\n")
    f.write("}\n")

print(f"Filtered opening book written to {OUTPUT_HEADER}")
