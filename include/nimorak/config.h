#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define TT_SIZE (1 << 26)
#define HISTORY_SIZE (1 << 20)
#define REPETITION_SIZE (1 << 20)

typedef struct
{
    struct {
        bool do_legal_move_filtering;
        bool do_only_captures;
    } movegen;

    struct {
        bool do_material;
        bool do_piece_squares;
        bool do_center_control;
        bool do_king_safety;
        bool do_pawn_structure;
    } eval;

    struct {
        bool do_quiescense;
        bool do_transpositions;
        bool do_killer_moves;
        bool do_heuristics;
        bool do_info;
        
        int initial_depth;
        int maximum_depth;
        int maximum_quiescense_depth;
    } search;
} Configuration;

#endif