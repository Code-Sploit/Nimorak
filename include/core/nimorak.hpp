#pragma once

#include <tables/constants.hpp>
#include <tables/helpers.hpp>
#include <utils/config.hpp>
#include <storage/transposition.hpp>
#include <storage/repetition.hpp>
#include <core/movegen.hpp>
#include <core/attack.hpp>
#include <core/eval.hpp>
#include <core/board.hpp>
#include <core/search.hpp>
#include <utils/tuning.hpp>
#include <memory>   // for std::unique_ptr
#include <string>

namespace Nimorak {
    class State {
        public:
            CastlingRights castlingRights;
            ZobristHash zobristKey;

            Piece capturedPiece;
            Move move;

            Bitboard attackMapFull[2];
            Bitboard attackMap[2][64];

            int enpassantSquare;
            int turn;

            Bitboard board[3][7];      // Pieces per type/side
            Bitboard occupancy[3];     // Side occupancy

            Piece boardGhost[64];      // Full piece array

            int fiftyMoveCounter;
    };

    class Game {
        public:
            const static size_t HISTORY_SIZE = 16384;
            // Game state
            int turn;
            int enpassantSquare;
            int historyCount;
            int isFirstLoad;

            CastlingRights castlingRights;

            Bitboard board[3][7];       // Pieces by type/side
            Bitboard occupancy[3];      // Side occupancy

            Movegen::MoveList movelist;

            Piece boardGhost[64];

            Transposition::Table transpositionTable;
            Repetition::Table repetitionTable;

            Config::Configuration config;

            Attack::Worker attackWorker;
            Movegen::Worker movegenWorker;
            Evaluation::Worker evalWorker;
            Search::Worker searchWorker;
            Tuning::Worker tuningWorker;

            ZobristHash zobristKey;

            std::unique_ptr<State[]> history;

            int winner;

            // Constructor / Destructor
            Game();
            ~Game(); // optional, default would work fine
    };
} // namespace Nimorak