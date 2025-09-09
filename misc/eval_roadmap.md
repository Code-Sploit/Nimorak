# Chess Engine Evaluation Roadmap

## 1. Material Evaluation
- [x] Sum piece values (TODO: phase-specific weighting).
- [ ] Include bishop pair bonus and knight vs. bishop dynamics.
- [ ] Consider rook on open file bonus.

## 2. Piece-Square Tables (PST)
- [x] Use PSTs for opening, middlegame, endgame.
- [ ] Refine king, pawn, and piece tables.
- [x] Mirror squares for black pieces.

## 3. Mobility
- [x] Count pseudo-legal moves per piece.
- [ ] Weight mobility by safety (avoid hanging pieces).
- [x] Adjust king mobility for endgame activity.

## 4. Pawn Structure
- [ ] Evaluate isolated pawns.
- [ ] Penalize doubled pawns.
- [ ] Reward passed pawns and connected pawns.
- [ ] Weight by game phase.

## 5. King Safety
- [ ] Evaluate pawn shield in front of king.
- [ ] Consider open files near king.
- [ ] Check attacking threats from opponent pieces.
- [ ] Adjust king centralization in endgame.

## 6. Tactical Threats
- [ ] Identify hanging or undefended pieces.
- [ ] Detect potential forks, skewers, pins.
- [ ] Include threats against king (checks, attacks).

## 7. Game Phase Awareness
- [x] Compute continuous game phase (opening → endgame).
- [ ] Interpolate piece values, PSTs, and evaluation weights dynamically.

## 8. Rook Activity
- [ ] Reward rooks on open or semi-open files.
- [ ] Penalize passive rooks blocked by own pawns.

## 9. Knight Outposts
- [ ] Reward knights on protected central squares.
- [ ] Penalize knights on rim or poorly supported squares.

## 10. Bishop Pair
- [ ] Give bonus for having both bishops.
- [ ] Scale bonus by game phase (middlegame emphasis).

## 11. Center Control
- [x] Reward control of central squares (e4, d4, e5, d5).
- [ ] Include influence over surrounding squares.

## 12. Passed Pawn Potential
- [ ] Evaluate distance of passed pawns from promotion.
- [ ] Reward connected and supported advancement.
- [ ] Penalize blocked passed pawns.

## 13. King Activity in Endgame
- [ ] Reward king centralization.
- [ ] Consider active king participation in pawn races.

## 14. Positional Weaknesses
- [ ] Detect backward pawns.
- [ ] Penalize weak squares and holes.
- [ ] Reward strong outposts and pawn chains.

## 15. Evaluation Integration
- [x] Modular design: enable/disable each module.
- [ ] Weight each module dynamically based on game phase.
- [ ] Allow tuning via self-play or test suites.