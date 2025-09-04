# Chess Engine Evaluation Roadmap

## Big Point 1: Basic Evaluation (ELO 800–1100)

Focus: Core material and basic positional awareness

* [x] **ELO 800** – Material balance (simple piece values)
* [x] **ELO 900** – Basic mobility: more moves = slightly better position
* [x] **ELO 950** – Simple king safety evaluation (penalize exposed king)
* [x] **ELO 1000** – Pawn structure basics: doubled and isolated pawns penalized
* [x] **ELO 1100** – Piece-square tables for basic positioning

ELO range for Big Point 1: 800–1100

## Big Point 2: Intermediate Evaluation (ELO 1200–1600)

Focus: Positional understanding and simple strategic concepts

* [ ] **ELO 1200** – Pawn structure features: passed pawns, backward pawns
* [ ] **ELO 1250** – King safety improvements (pawn shield, open files)
* [ ] **ELO 1300** – Control of key squares and central influence
* [ ] **ELO 1350** – Piece coordination: minor pieces working together
* [ ] **ELO 1400** – Rook activity and open/semi-open files
* [ ] **ELO 1450** – Minor piece outposts (advanced knight/bishop positioning)
* [ ] **ELO 1500** – Evaluating threats: attacking high-value targets
* [ ] **ELO 1550** – Mobility and flexibility evaluation (more options = better)
* [ ] **ELO 1600** – Endgame awareness: king activity, pawn promotion potential

ELO range for Big Point 2: 1200–1600

## Big Point 3: Advanced Evaluation (ELO 1700–2100)

Focus: Long-term positional evaluation, strategic imbalances

* [ ] **ELO 1700** – Strategic imbalances: pawn structure, bishop pairs, color weaknesses
* [ ] **ELO 1750** – Rook and queen coordination on key files/ranks
* [ ] **ELO 1800** – King safety in more complex positions (pawn storms, opposite-side castling)
* [ ] **ELO 1850** – Pawn majority evaluation and potential breakthroughs
* [ ] **ELO 1900** – Positional threats: prophylaxis and long-term planning
* [ ] **ELO 2000** – Piece activity evaluation in complex middlegames
* [ ] **ELO 2100** – Advanced positional evaluation: exchange sacrifices, weak squares, outposts

ELO range for Big Point 3: 1700–2100

## Big Point 4: Neural Network Evaluation (NNUE) (ELO 2200–2600)

Focus: Using neural networks to combine positional features automatically

* [ ] **ELO 2200** – NNUE integration for static evaluation
* [ ] **ELO 2300** – NNUE learns complex positional patterns beyond handcrafted rules
* [ ] **ELO 2400** – NNUE handles king safety, piece activity, pawn structures dynamically
* [ ] **ELO 2600** – NNUE optimized: integrates strategic planning, imbalances, and positional understanding automatically

ELO range for Big Point 4: 2200–2600