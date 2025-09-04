#include <core/tuning.hpp>
#include <core/nimorak.hpp>
#include <future>
#include <thread>
#include <vector>
#include <iostream>

namespace Tuning {

// -------------------------------
// Apply pawn weights to engine
// -------------------------------
void Worker::setPawnWeights(Nimorak::Game& game, const PawnWeights& weights)
{
    game.evalWorker.PAWN_DOUBLED_PENALTY  = weights.doubled;
    game.evalWorker.PAWN_BLOCKED_PENALTY  = weights.blocked;
    game.evalWorker.PAWN_ISOLATED_PENALTY = weights.isolated;
    game.evalWorker.PAWN_BACKWARD_PENALTY = weights.backward;
    game.evalWorker.PASSED_PAWN_BONUS     = weights.passedBase;
    game.evalWorker.PASSED_PAWN_RANK_MULT = weights.passedRank;
    game.evalWorker.PAWN_CONNECTED_BONUS  = weights.connected;
}

// -------------------------------
// Thread-safe self-play game
// -------------------------------
int Worker::playSelfGameThreadSafe(const PawnWeights& weights,
                                   const std::string& fen,
                                   int maxPlies)
{
    Nimorak::Game whiteGame, blackGame, thirdParty;

    Board::loadFen(whiteGame, fen);
    Board::loadFen(blackGame, fen);
    Board::loadFen(thirdParty, fen);

    setPawnWeights(whiteGame, weights);
    setPawnWeights(blackGame, weights);

    thirdParty.config.search.doInfo = false;

    while (!Board::isGameOver(thirdParty) &&
           !thirdParty.repetitionTable.checkFiftyMoveRule())
    {
        Move bestMove = 0;

        if (thirdParty.turn == WHITE)
            bestMove = whiteGame.searchWorker.searchPosition(thirdParty, 5, 35);
        else
            bestMove = blackGame.searchWorker.searchPosition(thirdParty, 5, 35);

        if (!bestMove) break;
        Board::makeMove(thirdParty, bestMove, MAKE_MOVE_FULL);
    }

    if (thirdParty.winner == WHITE) return 1;
    if (thirdParty.winner == BLACK) return -1;
    return 0;
}

// -------------------------------
// Parallel evaluation of weights
// -------------------------------
double Worker::evaluateWeightsParallel(const PawnWeights& weights,
                                       int gamesPerEval,
                                       int maxPlies,
                                       int& wins,
                                       int& losses,
                                       int& draws)
{
    int numThreads = std::max(1u, std::thread::hardware_concurrency());
    int gamesPerThread = (gamesPerEval + numThreads - 1) / numThreads;

    std::vector<std::future<std::tuple<int,int,int,int>>> futures;

    auto workerFunc = [&](int n) {
        int localScore=0, w=0, l=0, d=0;
        for (int i = 0; i < n; ++i)
        {
            std::string fen = fenStrings[i % fenStrings.size()];
            int result = playSelfGameThreadSafe(weights, fen, maxPlies);

            localScore += result;
            if (result == 1) ++w;
            else if (result == -1) ++l;
            else ++d;
        }
        return std::make_tuple(localScore, w, l, d);
    };

    for (int t = 0; t < numThreads; ++t)
        futures.push_back(std::async(std::launch::async, workerFunc, gamesPerThread));

    int totalScore=0; wins=0; losses=0; draws=0;
    for (auto& f : futures) {
        auto [s, w, l, d] = f.get();
        totalScore += s;
        wins += w;
        losses += l;
        draws += d;
    }

    return static_cast<double>(totalScore) / gamesPerEval;
}

// -------------------------------
// Main pawn weight tuner
// -------------------------------
void Worker::tunePawnWeights(Nimorak::Game& game,
                             Nimorak::Game& baseline,
                             int epochs,
                             int gamesPerEval)
{
    PawnWeights weights;
    static const char* paramNames[7] = {
        "Doubled", "Blocked", "Isolated", "Backward",
        "PassedBase", "PassedRank", "Connected"
    };

    int maxPlies = 60;

    for (int epoch=0; epoch<epochs; ++epoch)
    {
        int wins=0, losses=0, draws=0;
        double baseScore = evaluateWeightsParallel(weights, gamesPerEval, maxPlies, wins, losses, draws);

        PawnWeights bestWeights = weights;
        double bestScore = baseScore;

        std::cout << "\n=== Epoch " << epoch << " ===\n";
        std::cout << "BaseScore=" << baseScore
                  << " (W=" << wins << ", L=" << losses << ", D=" << draws << ")\n";

        for (int i=0; i<7; ++i)
        {
            std::cout << "  >> Tuning " << paramNames[i]
                      << " (current=" << weights[i] << ")\n";

            for (int delta : {-1, +1})
            {
                PawnWeights testWeights = weights;
                testWeights[i] += delta;

                int tWins=0, tLosses=0, tDraws=0;
                double score = evaluateWeightsParallel(testWeights, gamesPerEval, maxPlies, tWins, tLosses, tDraws);

                std::cout << "     Δ" << delta
                          << " -> score=" << score
                          << " (W=" << tWins << ", L=" << tLosses << ", D=" << tDraws << ")\n";

                if (score > bestScore)
                {
                    bestScore = score;
                    bestWeights = testWeights;
                    std::cout << "     ** Improvement found! " << bestScore << " -> " << score << "\n";
                }
            }
        }

        weights = bestWeights;

        std::cout << "Epoch " << epoch << " done.\n";
        std::cout << "Best score: " << bestScore << "\n";
        std::cout << "Best weights: "
                  << "Doubled=" << weights.doubled << ", "
                  << "Blocked=" << weights.blocked << ", "
                  << "Isolated=" << weights.isolated << ", "
                  << "Backward=" << weights.backward << ", "
                  << "PassedBase=" << weights.passedBase << ", "
                  << "PassedRank=" << weights.passedRank << ", "
                  << "Connected=" << weights.connected
                  << "\n";
    }
}

} // namespace Tuning