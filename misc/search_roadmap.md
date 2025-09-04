# Chess Engine Search Algorithm Roadmap

## Big Point 1: Basic Search (ELO 800–1200)

Focus: Simple depth-limited search and move generation

* [x] **ELO 800** – Generate all legal moves
* [x] **ELO 850** – Basic 1-ply search (evaluate immediate moves)
* [x] **ELO 900** – Simple depth-2 search with evaluation function
* [x] **ELO 950** – Store best move from depth-1 search
* [x] **ELO 1000** – Basic move ordering: prioritize captures first
* [x] **ELO 1050** – Detect checkmate/stalemate in search
* [x] **ELO 1100** – Evaluate leaf nodes using evaluation function
* [x] **ELO 1150** – Avoid illegal moves in search tree
* [x] **ELO 1200** – Implement iterative deepening (depth 1 → 2 → …)

ELO range for Big Point 1: 800–1200

## Big Point 2: Intermediate Search (ELO 1300–1700)

Focus: Efficient pruning, move ordering, and tactical awareness

* [x] **ELO 1300** – Alpha-beta pruning to reduce nodes searched
* [x] **ELO 1350** – Improve move ordering: captures, checks, threats first
* [x] **ELO 1400** – Quiescence search: extend tactical positions
* [ ] **ELO 1450** – Evaluate threats dynamically during search
* [x] **ELO 1500** – Transposition table to avoid recomputing positions
* [ ] **ELO 1550** – Detect and handle repetition draws
* [x] **ELO 1600** – Simple late-move reductions (search less promising moves shallower)
* [ ] **ELO 1650** – Implement iterative deepening with aspiration windows
* [ ] **ELO 1700** – Extend search for checks, captures, or threats

ELO range for Big Point 2: 1300–1700

## Big Point 3: Advanced Search (ELO 1800–2200)

Focus: Complex pruning, selective search, and endgame awareness

* [ ] **ELO 1800** – Null-move pruning to cut non-critical branches
* [ ] **ELO 1850** – Late-move reduction tuned for evaluation
* [ ] **ELO 1900** – Principal variation search (PV search) for better move ordering
* [ ] **ELO 1950** – Multi-cut / forward pruning for tactical positions
* [ ] **ELO 2000** – Endgame tablebase integration for perfect play in small-piece positions
* [ ] **ELO 2100** – Singular extension search for forced moves
* [ ] **ELO 2200** – Evaluate move safety and positional threats in search tree

ELO range for Big Point 3: 1800–2200

## Big Point 4: Neural Network & Modern Search (ELO 2300–2600)

Focus: Neural search evaluation and hybrid search techniques

* [ ] **ELO 2300** – NN-based static evaluation integrated into search (NNUE-style)
* [ ] **ELO 2350** – Move pruning guided by NN evaluation confidence
* [ ] **ELO 2400** – Hybrid search: combine alpha-beta with NN heuristics
* [ ] **ELO 2500** – Neural network predicts promising moves for selective search
* [ ] **ELO 2600** – Fully optimized search with NN-guided pruning and move ordering

ELO range for Big Point 4: 2300–2600