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

2) Late Move Reductions (LMR)

Reduce search depth for quiet, low-priority moves (i.e., after first N moves). Re-search at full depth if reduction produces a value > alpha.

Rules (common, simple):

If depth >= 3 and move is quiet and not a killer/tt/promotion/capture/check: reduce by R = 1 + (log(depth) * something) or simple R = 1 or 2.

Use incremental reduction: reducedDepth = depth - 1 - (depth>6 ? 1 : 0) etc.

Sketch:

bool isQuiet = !Helpers::is_capture(move) && !Helpers::is_promo(move) && !predictCheck(game, move);
bool isKiller = (move == killerMoves[ply][0] || move == killerMoves[ply][1]);
if (isQuiet && !isKiller && depth >= 3) {
    int red = 1 + (int)log2(depth); // simple formula
    Board::makeMove(game, move, MAKE_MOVE_FULL);
    int eval = -negamax(game, depth - 1 - red, -alpha-1, -alpha, ply+1);
    Board::unmakeMove(game, MAKE_MOVE_FULL);

    // If reduced search returns promising value, re-search full window
    if (eval > alpha && eval < beta) {
        Board::makeMove(game, move, MAKE_MOVE_FULL);
        eval = -negamax(game, depth - 1, -beta, -alpha, ply+1);
        Board::unmakeMove(game, MAKE_MOVE_FULL);
    }
} else {
    // normal full search
}


LMR + PVS together are extremely effective.

3) Improve move ordering: History heuristic and Counter-move

Your current ordering uses TT, captures (MVV-LVA), check bonus, killer moves — good. Add:

History table: int history[64][64] or int history[PIECE_TYPE][TO] — increment for moves that cause cutoffs, decay over time or bounded ints. Add history score to quiet moves.

Counter move table: Move counter[64][64] mapping last move to the best reply; give strong boost to that reply in ordering.

Sketch: after a cutoff, history[from][to] += depth*depth (common). Use history score in orderMoves for quiet moves.

Example addition in orderMoves:

if (!Helpers::is_capture(m) && !Helpers::is_promo(m)) {
    score += history[Helpers::get_from(m)][Helpers::get_to(m)];
}


On beta-cutoff update:

// when a move causes a cutoff
history[from][to] += depth * depth;
if (history[from][to] > HISTORY_MAX) history[from][to] = HISTORY_MAX;


Also set counter[lastMoveFrom][lastMoveTo] = moveThatCutoff.

4) Static Exchange Evaluation (SEE)

MVV-LVA is fast but crude — replace the "good/bad capture" decision with SEE to detect winning captures (i.e., those that gain material after recaptures). Also use SEE to decide which captures to generate in quiescence.

Pseudo:

int see = static_exchange_eval(game, move);
if (see >= 0) score += SEARCH_MOVE_CAPTURE_BIAS; // good capture
else score -= SEARCH_MOVE_CAPTURE_BIAS;


Implement SEE using attack/defender lists and iterative swapping; there are many compact implementations online. SEE reduces quiescence nodes and improves ordering.

5) Better Transposition Table (TT)

You probe and store already — ensure:

TT stores node type (EXACT/LOWER/UPPER), best move, depth, and age or generation counter to prefer replacing shallow entries with same age but prefer deeper ones.

Use two-entry or multi-entry per index (primary/secondary) with replacement policies: replace if new depth > stored depth or if older generation.

Use TT move as PV-first in ordering (you already do), but also consider internal iterative deepening to find better PV if TT move absent.

6) Quiescence search improvements

Only generate captures and promotions (and checks optionally), but prune captures with negative SEE (delta pruning) to avoid exploring obviously bad captures.

Implement delta pruning: if standPat + maxCaptureGain + margin <= alpha skip capture. maxCaptureGain is value of largest captured piece minus small margin. Example margin=200 centipawns.

Quiescence pseudo:

int stand = evaluate();
if (stand >= beta) return beta;
int maxGain = QUEEN_VALUE; // use board values
if (stand + maxGain + Q_MARGIN <= alpha) return alpha; // delta prune
generate captures only;
order by SEE/MVV-LVA;
for each capture:
    if (SEE(capture) < 0 && depth > 0) continue; // skip losing captures
    ...

7) Futility pruning & aspiration windows

At shallow depths (e.g., depth <= 2) if standPat + futility_margin <= alpha skip the node. Helps reduce nodes near leaves. Tune margins carefully.

Use aspiration windows around the previous iteration's score (e.g., ±50 cp). If search fails outside, do a full-window re-search. Speeds up search when score stable.

8) Singular extensions & selective extensions

Detect when a move is singular (much better than alternatives at depth-1) and extend by +1 ply to allow deeper analysis of tactics. Also always extend checks and promotions by +1.

Implement later — tests required; small Elo gains when implemented properly.

9) Evaluation upgrades (huge long-term gain)

Incremental evaluation: update eval incrementally on make/unmake (store material, pawn structure hashes, piece-square tables sums, bishop pair, mobility deltas). This gives big speedups (fewer full eval recomputations).

NNUE: if you want a large Elo jump, NNUE (efficient neural network input for evaluation) is the modern shortcut to strong eval without huge compute; requires training or using a pre-trained network. This is more work but high payoff.

10) Multi-threading / parallel search

Implement work stealing with shared TT and lockless probes (or fine-grained locks). Parallelization can raise practical playing strength with more cores but is more complex. Consider PVS+Young Brothers Wait Concept (YBWC) or shared kill counters.

Implementation notes & constants (practical defaults)

Null move reduction: commonly R = 2 for depths ≥ 3; be conservative in endgame (you already disable when few pieces — good). Consider if (depth > 2) R = 2; else no NMP.

LMR: reduce by 1 at depth 3–4, 2 at depth ≥ 5; or red = 1 + (int)log2(depth).

History increment on cutoff: history[from][to] += depth * depth.

Aspiration window default ±50–150 centipawns.

Quiescence depth: 20 moves? but better is to use a capture only limit (you already have maximumQuiescenseDepth constant).

TT size: allocate as many entries as memory allows. Use two-entry TT lines for fewer collisions.

Testing and tuning (must do this systematically)

Use an automated match harness (cutechess-cli widely used). Play long enough games (e.g., 30s+1m) or fixed-ply tests.

Run ablation tests: add one change at a time and run 300–1000 games to estimate Elo effect.

Use a stable Elo estimator (Bayesian or Glicko) or the cutechess built-in stats; don’t trust <100 games.

Use per-change parameter sweeps (history table scaling, LMR formulas, null-move reductions) to tune.

Quick checklist you can implement today (in order)

Add PVS (10–80 ELO depending on current ordering).

Add History table and include in orderMoves (10–50 ELO).

Implement LMR with a simple red = 1 + (depth>6) formula (10–50 ELO).

Replace capture "good/bad" heuristic with SEE for both ordering and quiescence (20–100 ELO).

Add aspiration windows at root (small speedup; reduces time).

Improve TT replacement policy (fewer hash collisions).

Add delta pruning in quiescence.

Small concrete code additions

History table declaration (e.g. in Worker or Game):

// globally or per-Search worker
static int history[64][64]; // from, to
// initialize to zero at engine start


Update on cutoff (inside negamax, when you detect a beta-cutoff):

if (eval >= beta) {
    // update history
    int from = Helpers::get_from(move);
    int to   = Helpers::get_to(move);
    history[from][to] += depth * depth;
    if (history[from][to] > 1000000) history[from][to] = 1000000;
    // set killers, store TT etc.
}


Use history in orderMoves (add after killers):

if (!Helpers::is_capture(m) && !Helpers::is_promo(m))
    score += history[Helpers::get_from(m)][Helpers::get_to(m)];

Pitfalls & gotchas

Null-move in zugzwang: you already disable NMP for pawn-only endgames — good. Also be careful in heavy-locked positions (blocked pawns).

Over-aggressive reductions/pruning can cause tactical blunders (ELO drop). Always test.

Threading: parallel search is tricky; get a strong single-threaded baseline first.

NNUE: great reward but needs different evaluation architecture and possibly SIMD support.

Final advice

If you only pick three things to implement now for the best cost/benefit:

PVS (fast, relatively simple).

History heuristic + enhance orderMoves (low risk, high payoff).

LMR (big node reduction, moderate risk).

After these, do SEE and quiescence/delta pruning. Once search is stable, invest in evaluation (incremental + NNUE) for the next big jump.