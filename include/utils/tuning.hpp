#pragma once

#include <string>
#include <tuple>
#include <vector>
#include <iostream>

namespace Rune {
    class Game;
}

namespace Tuning {
    class Worker {
        public:
            // -------------------------------
            // Struct to hold pawn evaluation weights
            // -------------------------------
            struct PawnWeights {
                int doubled     = 20;
                int blocked     = 14;
                int isolated    = 22;
                int backward    = 25;
                int passedBase  = 28;
                int passedRank  = 14;
                int connected   = 10;

                int& operator[](int i) {
                    switch(i) {
                        case 0: return doubled;
                        case 1: return blocked;
                        case 2: return isolated;
                        case 3: return backward;
                        case 4: return passedBase;
                        case 5: return passedRank;
                        case 6: return connected;
                        default: throw std::out_of_range("PawnWeights index out of range");
                    }
                }
            };

            // -------------------------------
            // FEN positions for training/tuning
            // -------------------------------
            std::vector<std::string> fenStrings = {
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                "r1bqkbnr/1p1ppppp/p1n5/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 1",
                "rnbqkbnr/2p1pppp/p7/1p6/2pPP3/5N2/PP3PPP/RNBQKB1R w KQkq - 0 1",
                "r2qkbnr/pbp2ppp/1pnp4/4p3/8/2N2NP1/PPPPPPBP/R1BQ1RK1 w kq - 0 1",
                "r1bqk2r/pppp1ppp/2n2n2/2b1p1B1/2B1P3/3P4/PPP2PPP/RN1QK1NR w KQkq - 0 1"
            };

            std::string getFenForIteration(int i);

            // -------------------------------
            // Apply pawn weights to engine
            // -------------------------------
            void setPawnWeights(Rune::Game& game, const PawnWeights& weights);

            // -------------------------------
            // Self-play evaluation (single-thread)
            // -------------------------------
            int playSelfGame(Rune::Game& game,
                            Rune::Game& baseline,
                            int maxPlies,
                            const std::string& fen);

            // -------------------------------
            // Self-play evaluation (thread-safe)
            // -------------------------------
            int playSelfGameThreadSafe(const PawnWeights& weights,
                                    const std::string& fen,
                                    int maxPlies);

            // -------------------------------
            // Evaluate a batch of games in parallel
            // -------------------------------
            double evaluateWeightsParallel(const PawnWeights& weights,
                                        int gamesPerEval,
                                        int maxPlies,
                                        int& wins,
                                        int& losses,
                                        int& draws);

            // -------------------------------
            // Main pawn weight tuner
            // -------------------------------
            void tunePawnWeights(Rune::Game& game,
                                Rune::Game& baseline,
                                int epochs,
                                int gamesPerEval);
    };
} // namespace Tuning