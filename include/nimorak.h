#ifndef NIMORAK_H
#define NIMORAK_H

#include <nimorak/constants.h>
#include <nimorak/helpers.h>
#include <nimorak/config.h>

typedef struct {
    int   count;
    Move  moves[256];
} MoveList;


typedef struct {
    CastlingRights castling_rights;
    ZobristHash    zobrist_key;
    Piece          captured_piece;
    Move           move;

    Bitboard       attack_map_full[2];
    Bitboard       attack_map[2][64];

    int            enpassant_square;
} State;


typedef struct {
    uint64_t  mask;
    uint64_t  magic;
    uint64_t *table;

    int       shift;
} Magic;


typedef struct {
    ZobristHash key;

    int         depth;
    int         eval;
    int         flag;

    Move        best_move;
} TTEntry;


typedef struct {
    ZobristHash stack[REPETITION_SIZE];
    int         count;
} RepetitionTable;

typedef struct {
    // Game state
    int              turn;
    int              enpassant_square;
    int              history_count;
    CastlingRights   castling_rights;

    // Board and occupancy
    Bitboard         board[3][7];              // [color][piece_type]
    Bitboard         occupancy[3];             // [color]
    
    // Attack data
    Bitboard         attack_map[2][64];        // [color][square]
    Bitboard         attack_map_full[2];       // [color]
    AttackTable      attack_tables_pc[7][64];  // [piece_type][square]

    // Move generation
    MoveList        *movelist;

    // Game history
    State            history[HISTORY_SIZE];
    Piece            board_ghost[64];

    // Transposition
    TTEntry          transposition_table[TT_SIZE];

    // Zobrist hashing
    ZobristHash      zobrist_key;

    // Repetition detection
    RepetitionTable *repetition_table;
} Game;

Game *game_new();

void game_del(Game *game);

#endif
