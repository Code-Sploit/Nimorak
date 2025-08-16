#ifndef NIMORAK_H
#define NIMORAK_H

#include <nimorak/constants.h>
#include <nimorak/helpers.h>
#include <nimorak/config.h>

#include <time.h>

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
    int            turn;            // also save turn here!
    
    Bitboard       board[3][7];
    Bitboard       occupancy[3];
    Piece          board_ghost[64];
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
    int         start;
    int         count;
} RepetitionTable;

typedef struct {
    // Game state
    int              turn;
    int              enpassant_square;
    int              history_count;
    int              board_is_first_load;
    CastlingRights   castling_rights;

    // Board and occupancy
    Bitboard         board[3][7];              // [color][piece_type]
    Bitboard         occupancy[3];             // [color]
    
    // Attack data
    Bitboard         attack_map_includes_square;
    Bitboard         attack_map[2][64];        // [color][square]
    Bitboard         attack_map_full[2];       // [color]
    AttackTable      attack_tables_pc[7][64];  // [piece_type][square]
    AttackTable      attack_tables_pc_pawn[2][64];
    uint8_t          attack_map_count[2][64];
    
    // Move generation
    MoveList        *movelist;

    // Game history
    State            history[HISTORY_SIZE];
    Piece            board_ghost[64];

    // Transposition
    TTEntry          transposition_table[TT_SIZE];

    // Castling rights
    uint8_t castling_rights_lookup[64][64];

    // Zobrist hashing
    ZobristHash      zobrist_key;

    // Repetition detection
    RepetitionTable *repetition_table;

    // Configuration
    Configuration *config;

    // Search
    clock_t          search_start_time;
    clock_t          search_last_depth_started_at;
    clock_t          search_last_depth_finished_at;
    int              search_think_time;
    int              search_stop;
} Game;

Game *game_new();

void game_del(Game *game);

int config_set_option(Game *game, const char *name, const char *value);
void config_handle_input(Game *game, const char *input);

#endif
