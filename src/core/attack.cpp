#include <core/board.hpp>

#include <tables/helpers.hpp>
#include <tables/magic.hpp>
#include <tables/zobrist.hpp>

#include <stdbool.h>
#include <stdlib.h>
#include <string>
#include <iostream>

static inline Bitboard attackGetSlider(PieceType type, int square, Bitboard occupancy)
{
    switch (type)
    {
        case BISHOP: return Magic::getBishopAttacks(square, occupancy);
        case ROOK:   return Magic::getRookAttacks(square, occupancy);
        case QUEEN:  return Magic::getQueenAttacks(square, occupancy);
        
        default: return 0ULL;
    }
}

namespace Attack {
    void Worker::generatePawns(Rune::Game& game, PieceColor color)
    {
        Bitboard pawns   = game.board[color][PAWN];

        while (pawns)
        {
            int square = __builtin_ctzll(pawns);

            pawns &= pawns - 1;

            Bitboard attacks = this->preComputed.pawns[color][square];

            this->attackMap[color][square] = attacks;
        }
    }

    void Worker::generateKnights(Rune::Game& game, PieceColor color)
    {
        Bitboard knights = game.board[color][KNIGHT];

        while (knights)
        {
            int square = __builtin_ctzll(knights);

            knights &= knights - 1;

            Bitboard attacks = this->preComputed.pieces[KNIGHT][square];
            
            this->attackMap[color][square] = attacks;
        }
    }

    void Worker::generateKing(Rune::Game& game, PieceColor color)
    {
        Bitboard king = game.board[color][KING];

        while (king)
        {
            int square = __builtin_ctzll(king);
            king &= king - 1;

            Bitboard attacks = this->preComputed.pieces[KING][square];
          
            this->attackMap[color][square] = attacks;
        }
    }

    void Worker::generateSliding(Rune::Game& game, PieceColor color, PieceType type)
    {
        Bitboard sliders = game.board[color][type];
        Bitboard occupancy = game.occupancy[BOTH];

        while (sliders)
        {
            int square = __builtin_ctzll(sliders);
            sliders &= sliders - 1;

            Bitboard attacks = 0ULL;
            switch (type)
            {
                case BISHOP: attacks = Magic::getBishopAttacks(square, occupancy); break;
                case ROOK:   attacks = Magic::getRookAttacks(square, occupancy); break;
                case QUEEN:  attacks = Magic::getQueenAttacks(square, occupancy); break;
                default: break;
            }

            this->attackMap[color][square] = attacks;
        }
    }

    void Worker::generateTable(Rune::Game& game, int side)
    {
        // Clear any previous per-square data for this side
        std::fill(&this->attackMap[side][0], &this->attackMap[side][0] + 64, 0ULL);
        this->attackMapFull[side] = 0ULL;

        // Generate per-piece attack maps (these helpers set this->attackMap[side][square])
        generatePawns(game, side);
        generateKnights(game, side);
        generateKing(game, side);
        generateSliding(game, side, BISHOP);
        generateSliding(game, side, ROOK);
        generateSliding(game, side, QUEEN);

        // Build full attack union for side — iterate all squares (safe & simple)
        for (int s = 0; s < 64; ++s) {
            this->attackMapFull[side] |= this->attackMap[side][s];
        }
    }

    void Worker::printTable(Rune::Game& game, int side)
    {
        std::cout << std::endl;

        for (int rank = 7; rank >= 0; rank--)
        {
            std::cout << " +---+---+---+---+---+---+---+---+" << std::endl << " ";

            for (int file = 0; file < 8; file++)
            {
                int square = rank * 8 + file;

                if ((this->attackMapFull[side] & (1ULL << square)) == 0)
                {
                    std::cout << "|   ";
                }
                else
                {
                    std::cout << "| X ";
                }
            }

            std::cout << "| " << rank + 1 << "  " << std::endl;
        }

        std::cout << " +---+---+---+---+---+---+---+---+  " << std::endl;
        std::cout << "  a   b   c   d   e   f   g   h " << std::endl;

        std::cout << "FEN: " << Board::generateFen(game) << std::endl;
        std::cout << "Zobrist Key: " << Zobrist::hashToString(game.zobristKey) << std::endl;
        std::cout << "Checkers: " << Board::getCheckers(game) << std::endl;
        std::cout << std::endl;
    }

    void Worker::generateAll(Rune::Game& game)
    {
        // Full occupancy must be preserved for every call to Magic::get*Attacks
        Bitboard fullOcc = game.occupancy[BOTH];

        // Clear everything
        std::fill(&this->attackMap[0][0], &this->attackMap[0][0] + 2*64, 0ULL);
        this->attackMapFull[WHITE] = 0ULL;
        this->attackMapFull[BLACK] = 0ULL;

        // Keep for debugging/consistency if you want to know which squares had pieces
        this->attackMapIncludes = fullOcc;

        // Iterate through the piece squares, but always pass fullOcc to sliding attack generators
        Bitboard iter = fullOcc;
        while (iter)
        {
            int square = __builtin_ctzll(iter);
            iter &= iter - 1;

            Piece piece = game.boardGhost[square];
            PieceColor color = Helpers::get_color(piece);
            PieceType type = Helpers::get_type(piece);

            Bitboard attacks = 0ULL;

            switch (type)
            {
                case PAWN:
                    attacks = this->preComputed.pawns[color][square];
                    break;
                case KNIGHT:
                    attacks = this->preComputed.pieces[KNIGHT][square];
                    break;
                case KING:
                    attacks = this->preComputed.pieces[KING][square];
                    break;
                case BISHOP:
                    attacks = Magic::getBishopAttacks(square, fullOcc);
                    break;
                case ROOK:
                    attacks = Magic::getRookAttacks(square, fullOcc);
                    break;
                case QUEEN:
                    attacks = Magic::getQueenAttacks(square, fullOcc);
                    break;
                default:
                    break;
            }

            this->attackMap[color][square] = attacks;
            this->attackMapFull[color] |= attacks;
        }
    }

    void Worker::update(Rune::Game& game, Move move)
    {
        const int from = Helpers::get_from(move);
        const int to   = Helpers::get_to(move);
        const Bitboard occ = game.occupancy[BOTH];

        const Piece movedPiece = game.boardGhost[to];
        const PieceColor color = Helpers::get_color(movedPiece);
        const PieceColor opponent = PieceColor(color ^ 1);

        // Determine the moved piece type (handle promotion)
        PieceType movedType = Helpers::get_type(movedPiece);
        if (Helpers::is_promo(move)) {
            movedType = Helpers::get_promo(move);
        }

        // --- Clear old attacks for moving piece ---
        this->attackMap[color][from] = 0ULL;

        // --- Clear opponent's attacks from 'to' square ---
        this->attackMap[opponent][to] = 0ULL;

        // --- Handle en passant ---
        int captureSq = to;
        if (Helpers::is_enpassant(move)) {
            captureSq = (color == WHITE) ? (to - 8) : (to + 8);
        }

        // --- Clear attacks of captured piece ---
        Piece capturedPiece = game.boardGhost[captureSq];
        if (Helpers::get_type(capturedPiece) != EMPTY) {
            PieceColor capColor = Helpers::get_color(capturedPiece);
            this->attackMap[capColor][captureSq] = 0ULL;
        } else if (Helpers::is_enpassant(move)) {
            this->attackMap[opponent][captureSq] = 0ULL;
        }

        // --- Add new attacks for moved piece ---
        Bitboard newAtt = 0ULL;
        switch (movedType) {
            case BISHOP: newAtt = Magic::getBishopAttacks(to, occ); break;
            case ROOK:   newAtt = Magic::getRookAttacks(to, occ); break;
            case QUEEN:  newAtt = Magic::getBishopAttacks(to, occ) | Magic::getRookAttacks(to, occ); break;
            case PAWN:   newAtt = this->preComputed.pawns[color][to]; break;
            case KNIGHT: newAtt = this->preComputed.pieces[KNIGHT][to]; break;
            case KING:   newAtt = this->preComputed.pieces[KING][to]; break;
            default: break;
        }
        this->attackMap[color][to] = newAtt;

        // --- Update sliders affected by from/to moves ---
        Bitboard sliderOccupancy =
            game.board[WHITE][BISHOP] | game.board[WHITE][ROOK] | game.board[WHITE][QUEEN] |
            game.board[BLACK][BISHOP] | game.board[BLACK][ROOK] | game.board[BLACK][QUEEN];

        Bitboard affected =
            sliderOccupancy & (
                Magic::getBishopAttacks(from, occ) | Magic::getBishopAttacks(to, occ) |
                Magic::getRookAttacks(from, occ)   | Magic::getRookAttacks(to, occ)
            );

        while (affected) {
            int sq = __builtin_ctzll(affected);
            affected &= affected - 1;

            Piece p = game.boardGhost[sq];
            PieceColor c = Helpers::get_color(p);
            PieceType pt = Helpers::get_type(p);

            if (pt == BISHOP)       newAtt = Magic::getBishopAttacks(sq, occ);
            else if (pt == ROOK)    newAtt = Magic::getRookAttacks(sq, occ);
            else if (pt == QUEEN)   newAtt = Magic::getBishopAttacks(sq, occ) | Magic::getRookAttacks(sq, occ);
            else continue;

            this->attackMap[c][sq] = newAtt;
        }

        // --- Update pawns, knights, king for moving color ---
        Bitboard pcs;

        pcs = game.board[color][PAWN];
        while (pcs) {
            int sq = __builtin_ctzll(pcs);
            pcs &= pcs - 1;
            this->attackMap[color][sq] = this->preComputed.pawns[color][sq];
        }

        pcs = game.board[color][KNIGHT];
        while (pcs) {
            int sq = __builtin_ctzll(pcs);
            pcs &= pcs - 1;
            this->attackMap[color][sq] = this->preComputed.pieces[KNIGHT][sq];
        }

        pcs = game.board[color][KING];
        if (pcs) {
            int sq = __builtin_ctzll(pcs);
            this->attackMap[color][sq] = this->preComputed.pieces[KING][sq];
        }

        // --- Recompute full attack map for both colors ---
        for (int c = 0; c < 2; c++) {
            Bitboard unionAtt = 0ULL;
            for (int s = 0; s < 64; s++) {
                unionAtt |= this->attackMap[c][s];
            }
            this->attackMapFull[c] = unionAtt;
        }
    }

    bool Worker::isSquareAttackedBy(int square, int color)
    {
        return (this->attackMapFull[color] & (1ULL << square)) != 0;
    }

    Bitboard Worker::getNewAttacksForMove(Rune::Game& game, Move move)
    {
        int from = Helpers::get_from(move);
        int to   = Helpers::get_to(move);

        Piece fPiece = game.boardGhost[from];

        int type  = Helpers::get_type(fPiece);
        int color = Helpers::get_color(fPiece);

        switch (type)
        {
            case PAWN: return this->preComputed.getPawnAttacks(color, to); break;
            case KNIGHT: return this->preComputed.getKnightAttacks(to); break;
            case BISHOP: return Magic::getBishopAttacks(to, game.occupancy[BOTH]); break;
            case ROOK: return Magic::getRookAttacks(to, game.occupancy[BOTH]); break;
            case QUEEN: return Magic::getQueenAttacks(to, game.occupancy[BOTH]); break;
            case KING: return this->preComputed.getKingAttacks(to); break;
            default: return 0ULL; break;
        }

        return 0ULL;
    }

    Bitboard Worker::getAttackersForSquare(Rune::Game& game, int color, int target)
    {
        Bitboard attackers = 0;
        Bitboard squareMask = (1ULL << target);
        Bitboard occupancy = game.occupancy[color];

        while (occupancy)
        {
            int sq = Helpers::pop_lsb(occupancy);

            if (attackMap[color][sq] & squareMask)
                attackers |= (1ULL << sq);
        }
        
        return attackers;
    }

    Bitboard Worker::getAttackersForZone(Rune::Game& game, Bitboard zone, int color, int type)
    {
        Bitboard attackers = 0;
        Bitboard pieces = game.board[color][type]; // all pieces of this type

        while (pieces) {
            int sq = Helpers::pop_lsb(pieces);

            if (attackMap[color][sq] & zone)  // piece attacks zone
                attackers |= (1ULL << sq);
        }

        return attackers; // squares of attacking pieces
    }
}