# Rune Chess Engine

<img src="misc/mdpicture.jpg" alt="Rune Engine Logo" width="200"/>

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/Code-Sploit/Rune)](https://github.com/Code-Sploit/Rune/releases)

**Rune** is a high-performance open-source chess engine written in C++. It supports the Universal Chess Interface (UCI) protocol and is designed for speed, accuracy, and configurability.

---

## Table of Contents
- [First Release Highlights](#first-release-highlights)
- [Features](#features)
- [Search Algorithm](#search-algorithm)
- [Evaluation Algorithm](#evaluation-algorithm)
- [Installation](#installation)
- [Usage](#usage)
- [UCI Commands](#uci-commands)
- [Configuration](#configuration)
- [Development](#development)
- [License](#license)

---

## First Release Highlights

| Feature | Description |
|---------|-------------|
| UCI Protocol | Full support for GUI integration |
| Opening Book | Precomputed openings with filtering of weak lines |
| Move Generation | Advanced and accurate move generation |
| Multi-threaded Search | Parallel search support |
| Evaluation | Material, piece-square tables, mobility, bishop pair, pawn structure, king safety |
| Perft Testing | Debugging and move counting |

---

## Features

- Full UCI protocol support
- Advanced move generation and evaluation
- Opening book support
- Multi-threaded search
- Perft testing for debugging
- Configurable evaluation parameters:
  - Material, piece-square tables, mobility, bishop pair, pawn structure, king safety
- Search options:
  - Quiescence, transposition table, iterative deepening, beta cutoff history
- Custom commands for debugging and board display

---

## Search Algorithm

Rune uses a **Negamax search framework with alpha-beta pruning**, enhanced with several modern techniques:

- **Iterative Deepening**: Searches incrementally from shallow to maximum depth to improve move ordering and time management.
- **Principal Variation Search (PVS)**: Optimizes the alpha-beta search by searching the first move with a full window and others with a null window.
- **Quiescence Search**: Extends search at leaf nodes to capture tactical moves and avoid horizon effect.
- **Transposition Table**: Stores previously computed positions to avoid redundant calculations.
- **Null-Move Pruning**: Skips moves when safe to quickly identify pruning opportunities.
- **Move Ordering**: Prioritizes moves using TT move, captures (MVV-LVA), checks, promotions, and beta cutoff history.
- **Static Exchange Evaluation (SEE)**: Estimates material gain for potential captures to improve move ordering.
- **Time Management**: Adjustable think time per move with monitoring for timeouts.

This combination ensures Rune can find the strongest moves efficiently while balancing search depth and tactical accuracy.

---

## Evaluation Algorithm

Rune's evaluation is **modular and phase-aware**, combining several heuristics:

- **Material Evaluation**: Counts pieces with standard values and adjusts for side-to-move perspective.
- **Piece-Square Tables (PST)**: Scores piece placement differently for opening, middlegame, and endgame phases.
- **Mobility**: Rewards pieces with more legal moves; penalizes king mobility outside endgame.
- **Pawn Structure**: Rewards passed pawns, penalizes doubled or backward pawns.
- **Bishop Pair**: Bonus for possessing a pair of bishops, scaled by game phase.
- **King Safety**:
  - Evaluates pawn shield in front of castled king.
  - Penalizes open/semi-open files near king.
  - Considers enemy attacks and central king exposure.
- **Game Phase Detection**: Dynamically determines if the position is in opening, middlegame, or endgame to scale evaluation weights appropriately.

This modular system ensures balanced positional understanding, tactical awareness, and smooth transition between game phases.

---

## Installation

Clone the repository:

```bash
git clone https://github.com/YourUsername/Rune.git
cd Rune
