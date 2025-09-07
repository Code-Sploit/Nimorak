#include <utils/tuning.hpp>
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
    //game.evalWorker.PAWN_DOUBLED_PENALTY  = weights.doubled;
    //game.evalWorker.PAWN_BLOCKED_PENALTY  = weights.blocked;
    //game.evalWorker.PAWN_ISOLATED_PENALTY = weights.isolated;
    //game.evalWorker.PAWN_BACKWARD_PENALTY = weights.backward;
    //game.evalWorker.PASSED_PAWN_BONUS     = weights.passedBase;
    //game.evalWorker.PASSED_PAWN_RANK_MULT = weights.passedRank;
    //game.evalWorker.PAWN_CONNECTED_BONUS  = weights.connected;
}

// --- playSelfGameThreadSafe: enforce maxPlies, pass weights to search, minor defensive checks
int Worker::playSelfGameThreadSafe(const PawnWeights& weights,
                                   const std::string& fen,
                                   int maxPlies)
{
    Nimorak::Game whiteGame, blackGame, thirdParty;

    Board::loadFen(whiteGame, fen);
    Board::loadFen(blackGame, fen);
    Board::loadFen(thirdParty, fen);

    // Apply weights to the searchers we will use.
    setPawnWeights(whiteGame, weights);
    setPawnWeights(blackGame, weights);

    thirdParty.config.search.doInfo = false;

    int plyCount = 0;
    while (!Board::isGameOver(thirdParty) &&
           !thirdParty.repetitionTable.checkFiftyMoveRule() &&
           plyCount < maxPlies)
    {
        Move bestMove = 0;

        // Use maxPlies (or some search-depth derived from it) when calling searchPosition.
        // The exact mapping between maxPlies and search parameters depends on your search API.
        // Here we pass maxPlies as the depth parameter; if your API expects time or nodes,
        // change accordingly.
        if (thirdParty.turn == WHITE)
            bestMove = whiteGame.searchWorker.searchPosition(thirdParty, /*depth=*/maxPlies, /*nodesOrLimit=*/50);
        else
            bestMove = blackGame.searchWorker.searchPosition(thirdParty, /*depth=*/maxPlies, /*nodesOrLimit=*/50);

        if (!bestMove) break;
        Board::makeMove(thirdParty, bestMove, MAKE_MOVE_FULL);
        ++plyCount;
    }

    if (thirdParty.winner == WHITE) return 1;
    if (thirdParty.winner == BLACK) return -1;
    return 0;
}

// --- evaluateWeightsParallel: proper per-thread distribution, avoid biased averaging,
// reduce fen correlation by offsetting, and divide by actual games played.
double Worker::evaluateWeightsParallel(const PawnWeights& weights,
                                       int gamesPerEval,
                                       int maxPlies,
                                       int& wins,
                                       int& losses,
                                       int& draws)
{
    int numThreads = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    // don't create more threads than games
    if (numThreads > gamesPerEval) numThreads = gamesPerEval;

    // compute exact per-thread counts that sum to gamesPerEval
    int base = gamesPerEval / numThreads;
    int remainder = gamesPerEval % numThreads;
    std::vector<int> perThreadCounts(numThreads, base);
    for (int i = 0; i < remainder; ++i) perThreadCounts[i]++;

    std::vector<std::future<std::tuple<int,int,int,int>>> futures;
    futures.reserve(numThreads);

    // Launch tasks, each gets a starting index offset so threads don't all replay same FEN order
    int globalStart = 0;
    for (int t = 0; t < numThreads; ++t)
    {
        int n = perThreadCounts[t];
        int startIndex = globalStart;
        globalStart += n;

        auto workerFunc = [this, &weights, n, startIndex, maxPlies]() -> std::tuple<int,int,int,int> {
            int localScore = 0, w = 0, l = 0, d = 0;
            const size_t fenCount = fenStrings.size();
            for (int i = 0; i < n; ++i)
            {
                // Choose fen in a staggered way so different threads hit different positions
                const std::string& fen = fenStrings[(startIndex + i) % fenCount];

                int result = playSelfGameThreadSafe(weights, fen, maxPlies);

                localScore += result;
                if (result == 1) ++w;
                else if (result == -1) ++l;
                else ++d;
            }
            return std::make_tuple(localScore, w, l, d);
        };

        futures.push_back(std::async(std::launch::async, workerFunc));
    }

    int totalScore = 0;
    wins = losses = draws = 0;
    int totalPlayed = 0;
    for (auto& f : futures) {
        auto [s, w, l, d] = f.get();
        int played = w + l + d;
        totalScore += s;
        wins += w;
        losses += l;
        draws += d;
        totalPlayed += played;
    }

    if (totalPlayed == 0) return 0.0;
    return static_cast<double>(totalScore) / static_cast<double>(totalPlayed);
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

            for (int delta : {-10, +10})
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
                    double oldBest = bestScore;
                    bestScore = score;
                    bestWeights = testWeights;
                    std::cout << "     ** Improvement found! " << oldBest << " -> " << bestScore << "\n";
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