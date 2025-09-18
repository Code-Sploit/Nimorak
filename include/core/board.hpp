#pragma once

#include <string>
#include <core/rune.hpp>
#include <tables/helpers.hpp>
#include <core/attack.hpp>

namespace Rune {
    class Game; // forward declaration
}

namespace Board {
    static constexpr Bitboard FILE_MASKS[8] = {
                0x0101010101010101ULL,
                0x0202020202020202ULL,
                0x0404040404040404ULL,
                0x0808080808080808ULL,
                0x1010101010101010ULL,
                0x2020202020202020ULL,
                0x4040404040404040ULL,
                0x8080808080808080ULL
    };
    // ----------------------------
    // FEN & Board helpers
    // ----------------------------
    std::string generateFen(Rune::Game& game);
    void loadFen(Rune::Game& game, const std::string fenString);

    inline std::string squareToString(int square)
    {
        int file = Helpers::file_of(square); // 0-7
        int rank = Helpers::rank_of(square); // 0-7

        std::string s;
        s += static_cast<char>('a' + file);    // file letter
        s += static_cast<char>('1' + rank);    // rank number

        return s;
    }

    inline int pieceTypeFromChar(char c)
    {
        switch (c)
        {
            case 'N': return KNIGHT;
            case 'B': return BISHOP;
            case 'R': return ROOK;
            case 'Q': return QUEEN;
            case 'K': return KING;
            default: return EMPTY;
        }
    }

    std::string getCheckers(Rune::Game& game);
    void print(Rune::Game& game);

    bool isOnRank(int square, int rank);
    bool isOnFile(int square, int file);

    std::string squareToName(int square);
    std::string moveToString(Move move);
    Move parseMove(Rune::Game& game, const std::string& moveStr);

    inline void setSquare(Rune::Game& game, int square, Piece piece);

    // ----------------------------
    // Move legality & castling
    // ----------------------------
    void makeMove(Rune::Game& game, Move move, int callType);
    void unmakeMove(Rune::Game& game, int callType);

    void makeNullMove(Rune::Game& game);
    void unmakeNullMove(Rune::Game& game);

    bool hasCastlingRights(Rune::Game& game, int side);
    bool hasCastlingRightsSide(Rune::Game& game, int side);

    bool isSameLine(int from, int to, int offset);
    int findKing(Rune::Game& game, int color);
    bool isKingInCheck(Rune::Game& game, int color);
    bool moveGivesCheck(Rune::Game& game, Move move);

    Bitboard getSlidingPiecesBitboard(Rune::Game& game, int color);

    bool hasNonPawnMaterial(Rune::Game& game, int color);

    void skipTurn(Rune::Game& game);
    void undoSkipTurn(Rune::Game& game);

    bool hasFullMaterial(Rune::Game& game, int color);
    bool pawnChainsLocked(Rune::Game& game);

    int countPieces(Rune::Game& game, PieceType type);
    int hasPiece(Rune::Game& game, PieceType type, PieceColor color);
    int totalMaterial(Rune::Game& game);

    bool isFileOpen(Rune::Game& game, int file);
    bool isFileSemiOpen(Rune::Game& game, int file, int color);

    int getPhase(Rune::Game& game);

} // namespace Board