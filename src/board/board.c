#include <table/repetition.h>
#include <table/zobrist.h>

#include <board/attack.h>
#include <board/board.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// Set a piece on a square, updating both bitboards and board_ghost
// Inline-able, branch-minimized set_square function
inline void board_set_square(Game *game, int square, Piece piece)
{
    // Assume caller ensures square in [0,63] for max speed.
    // If safety needed, keep this:
    if ((unsigned)square >= 64) return;

    const uint64_t bit = 1ULL << square;
    const uint64_t mask = ~bit;

    Piece old_piece = game->board_ghost[square];
    int old_type = GET_TYPE(old_piece);

    if (old_type != EMPTY) {
        int old_color = GET_COLOR(old_piece);
        game->board[old_color][old_type] &= mask;
        game->occupancy[old_color] &= mask;
    }

    game->board_ghost[square] = piece;

    int new_type = GET_TYPE(piece);
    if (new_type != EMPTY) {
        int new_color = GET_COLOR(piece);
        game->board[new_color][new_type] |= bit;
        game->occupancy[new_color] |= bit;
    }

    // Update combined occupancy at the end
    game->occupancy[BOTH] = game->occupancy[WHITE] | game->occupancy[BLACK];
}

// Inline fast get_square without checks (if caller guarantees valid input)
inline Piece board_get_square(const Game *game, int square)
{
    // Optionally keep safety check if needed:
    // if (!game || (unsigned)square >= 64) return MAKE_PIECE(EMPTY, EMPTY);
    return game->board_ghost[square];
}

// Inline fast check_square, no safety checks
int board_check_square(const Game *game, int square)
{
    // Optionally safety:
    // if (!game || (unsigned)square >= 64) return 0;

    return GET_TYPE(game->board_ghost[square]) != EMPTY;
}

void board_array_from_bitboards(Game *game) {
    if (!game) return;

    for (int sq = 0; sq < 64; sq++) {
        game->board_ghost[sq] = MAKE_PIECE(EMPTY, EMPTY); // Default: empty

        uint64_t mask = 1ULL << sq;

        // Use more efficient order: loop over color, then piece type
        for (int color = 0; color <= 1; color++) {
            uint64_t *pieces = game->board[color];
            
            if (pieces[PAWN]   & mask) { game->board_ghost[sq] = MAKE_PIECE(PAWN, color);   break; }
            if (pieces[KNIGHT] & mask) { game->board_ghost[sq] = MAKE_PIECE(KNIGHT, color); break; }
            if (pieces[BISHOP] & mask) { game->board_ghost[sq] = MAKE_PIECE(BISHOP, color); break; }
            if (pieces[ROOK]   & mask) { game->board_ghost[sq] = MAKE_PIECE(ROOK, color);   break; }
            if (pieces[QUEEN]  & mask) { game->board_ghost[sq] = MAKE_PIECE(QUEEN, color);  break; }
            if (pieces[KING]   & mask) { game->board_ghost[sq] = MAKE_PIECE(KING, color);   break; }
        }
    }
}

void board_load_fen(Game *game, const char *fen_string)
{
    if (!game || !fen_string) return;

    // Clear all bitboards
    memset(game->board, 0, sizeof(game->board));

    const char *ptr = fen_string;
    int square = 56; // Start at A8 (top-left)

    // 1. Parse piece placement
    while (*ptr && *ptr != ' ')
    {
        char c = *ptr;

        if (isdigit(c))
        {
            int empty = c - '0';
            for (int i = 0; i < empty; i++)
            {
                board_set_square(game, square, MAKE_PIECE(EMPTY, EMPTY));
                square++;
            }
        }
        else if (c == '/')
        {
            square -= 16; // Move to start of next rank
        }
        else
        {
            int color = isupper(c) ? WHITE : BLACK;
            int type;

            switch (tolower(c))
            {
                case 'p': type = PAWN; break;
                case 'n': type = KNIGHT; break;
                case 'b': type = BISHOP; break;
                case 'r': type = ROOK; break;
                case 'q': type = QUEEN; break;
                case 'k': type = KING; break;
                default:  type = EMPTY; break;
            }

            board_set_square(game, square, MAKE_PIECE(type, color));
            square++;
        }

        ptr++;
    }

    // Skip space
    if (*ptr == ' ') ptr++;

    // 2. Parse turn
    game->turn = (*ptr == 'w') ? WHITE : BLACK;

    while (*ptr && *ptr != ' ') ptr++;
    if (*ptr == ' ') ptr++;

    // 3. Parse castling rights
    game->castling_rights = 0;

    if (*ptr == '-')
    {
        ptr++;
    }
    else
    {
        while (*ptr && *ptr != ' ')
        {
            switch (*ptr)
            {
                case 'K': game->castling_rights |= 0x1; break; // White kingside
                case 'Q': game->castling_rights |= 0x2; break; // White queenside
                case 'k': game->castling_rights |= 0x4; break; // Black kingside
                case 'q': game->castling_rights |= 0x8; break; // Black queenside
            }
            ptr++;
        }
    }

    // Skip to end (en passant, halfmove clock, fullmove number ignored for now)
    // Can be added here if needed

    // Recalculate attack tables
    attack_generate_table(game, WHITE);
    attack_generate_table(game, BLACK);
}

void board_print(Game *game)
{
    if (!game) return;

    const char piece_chars[7] = {'.', 'P', 'N', 'B', 'R', 'Q', 'K'};

    printf("\n");

    for (int rank = 7; rank >= 0; rank--)
    {
        printf("%d  ", rank + 1);

        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;

            Piece p = board_get_square(game, square);

            if (p == 0)
            {
                printf(". ");
            }
            else
            {
                int type = p & 0x07;
                int color = (p & 0x08) >> 3;

                char c = piece_chars[type];
                if (color == 1) c = tolower(c);

                printf("%c ", c);
            }
        }

        printf("\n");
    }

    printf("\n   a b c d e f g h\n");

    printf("\nTurn: %s\n", game->turn == 0 ? "White" : "Black");

    printf("Castling rights: ");
    if (game->castling_rights & 0x1) printf("K");
    if (game->castling_rights & 0x2) printf("Q");
    if (game->castling_rights & 0x4) printf("k");
    if (game->castling_rights & 0x8) printf("q");
    if (game->castling_rights == 0) printf("-");

    printf("\n");
}

void board_make_move(Game *game, Move move)
{
    if (!game) return;

    const int from = GET_FROM(move);
    const int to = GET_TO(move);

    Piece piece = board_get_square(game, from);
    const int color = GET_COLOR(piece);

    // Capture detection - avoid branch by using ternary early
    const int ep_capture_sq = (color == WHITE) ? to - 8 : to + 8;
    Piece captured = IS_ENPASSANT(move) ? board_get_square(game, ep_capture_sq) : board_get_square(game, to);

    // Save current state for undo efficiently by stack allocation
    State s = {
        .castling_rights = game->castling_rights,
        .enpassant_square = game->enpassant_square,
        .captured_piece = captured,
        .move = move
    };
    
    memcpy(s.attack_map, game->attack_map, sizeof(game->attack_map));
    memcpy(s.attack_map_full, game->attack_map_full, sizeof(game->attack_map_full));

    s.zobrist_key = game->zobrist_key;

    game->history[game->history_count++] = s;

    // Move piece: clear from-square, place piece at to-square
    board_set_square(game, from, EMPTY);

    // Handle promotion inline to avoid double set_square calls
    if (IS_PROMO(move)) {
        const int promo_type = GET_PROMO(move);
        board_set_square(game, to, MAKE_PIECE(promo_type, color));
    } else {
        board_set_square(game, to, piece);
    }

    // Handle en passant capture pawn removal
    if (IS_ENPASSANT(move)) {
        board_set_square(game, ep_capture_sq, EMPTY);
    }

    // Castling rook move - simplified with precomputed offsets
    if (IS_CASTLE(move)) {
        int rook_from, rook_to;
        if (to == from + 2) {         // Kingside
            rook_from = from + 3;
            rook_to = from + 1;
        } else {                     // Queenside
            rook_from = from - 4;
            rook_to = from - 1;
        }
        Piece rook = board_get_square(game, rook_from);
        board_set_square(game, rook_from, EMPTY);
        board_set_square(game, rook_to, rook);
    }

    // Update castling rights - use a fast lookup table or bitmask checks
    const int piece_type = GET_TYPE(piece);
    if (piece_type == KING) {
        game->castling_rights &= ~(color == WHITE ? (WHITE_KINGSIDE | WHITE_QUEENSIDE) : (BLACK_KINGSIDE | BLACK_QUEENSIDE));
    }
    else if (piece_type == ROOK) {
        // Update castling rights depending on rook's original square (only 4 squares matter)
        switch (from) {
            case 0:  game->castling_rights &= ~WHITE_QUEENSIDE; break;
            case 7:  game->castling_rights &= ~WHITE_KINGSIDE; break;
            case 56: game->castling_rights &= ~BLACK_QUEENSIDE; break;
            case 63: game->castling_rights &= ~BLACK_KINGSIDE; break;
        }
    }
    if (captured != EMPTY && GET_TYPE(captured) == ROOK) {
        // Same for captured rook squares
        switch (to) {
            case 0:  game->castling_rights &= ~WHITE_QUEENSIDE; break;
            case 7:  game->castling_rights &= ~WHITE_KINGSIDE; break;
            case 56: game->castling_rights &= ~BLACK_QUEENSIDE; break;
            case 63: game->castling_rights &= ~BLACK_KINGSIDE; break;
        }
    }

    // Set en passant target or clear
    game->enpassant_square = IS_DOUBLE_PUSH(move) ? (color == WHITE ? to - 8 : to + 8) : -1;

    // Update attack tables - incremental updates are best for speed
    attack_generate_table(game, game->turn);
    attack_generate_table(game, !game->turn);

    // Flip side to move
    game->turn ^= 1;

    zobrist_update_board(game);

    repetition_push(game, game->zobrist_key);
}


void board_unmake_move(Game *game, Move move)
{
    if (!game) return;

    if (game->history_count <= 0) {
        fprintf(stderr, "Error: unmake_move with empty history!\n");
        exit(EXIT_FAILURE);
    }

    State *s = &game->history[--game->history_count];
    int from = GET_FROM(move);
    int to = GET_TO(move);

    Piece piece = board_get_square(game, to);
    int color = GET_COLOR(piece);

    // Undo promotion: revert to pawn if promo
    piece = IS_PROMO(move) ? MAKE_PIECE(PAWN, color) : piece;

    // Clear destination square first (to avoid overwrite conflicts)
    board_set_square(game, to, EMPTY);

    // Restore moved piece to from-square
    board_set_square(game, from, piece);

    // Restore captured piece (handle en passant separately)
    if (IS_ENPASSANT(move)) {
        int ep_sq = (color == WHITE) ? to - 8 : to + 8;
        board_set_square(game, ep_sq, s->captured_piece);
    } else if (s->captured_piece != EMPTY) {
        board_set_square(game, to, s->captured_piece);
    }

    // Undo castling rook move if applicable
    if (IS_CASTLE(move)) {
        // Kingside or queenside castling offset logic simplified
        int rook_from, rook_to;
        if (to == from + 2) { // kingside
            rook_from = from + 1;
            rook_to = from + 3;
        } else { // queenside
            rook_from = from - 1;
            rook_to = from - 4;
        }
        Piece rook = board_get_square(game, rook_from);
        board_set_square(game, rook_to, rook);
        board_set_square(game, rook_from, EMPTY);
    }

    // Restore castling rights and en passant square
    game->castling_rights = s->castling_rights;
    game->enpassant_square = s->enpassant_square;

    // Restore attack tables (copy with memcpy for speed)
    memcpy(game->attack_map, s->attack_map, sizeof(game->attack_map));
    memcpy(game->attack_map_full, s->attack_map_full, sizeof(game->attack_map_full));

    // Flip turn back
    game->turn ^= 1;

    game->zobrist_key = s->zobrist_key;

    repetition_pop(game);

    // Optional: you can skip clearing old state for speed
}

int board_is_on_rank(int square, int rank)
{
    // Assuming square is always valid; remove bounds check for speed
    return ((square >> 3) == rank);
}

int board_find_king(const Game *game, int color)
{
    // Assuming game->board[color][KING] always valid
    uint64_t king_bb = game->board[color][KING];
    return king_bb ? __builtin_ctzll(king_bb) : -1;
}

int board_is_king_in_check(const Game *game, int color)
{
    int king_square = board_find_king(game, color);
    if ((unsigned)king_square >= 64) return 0;  // safer cast & check

    AttackTable attacks = game->attack_map_full[!color];
    return (attacks & (1ULL << king_square)) != 0;
}

const char *board_square_to_name(int square)
{
    static char names[8][3];
    static int index = 0;

    if (square < 0 || square >= 64)
    {
        return "--";
    }

    int file = square % 8;
    int rank = square / 8;

    // Use circular buffer to avoid overwrite
    char *name = names[index];
    index = (index + 1) % 8;

    name[0] = 'a' + file;
    name[1] = '1' + rank;
    name[2] = '\0';

    return name;
}

const char *board_move_to_string(Move move)
{
    // circular buffer of 8 strings, each max 6 chars + '\0'
    #define MOVE_STR_BUFFERS 8
    static char move_strs[MOVE_STR_BUFFERS][6];
    static int index = 0;

    char *move_str = move_strs[index];
    index = (index + 1) % MOVE_STR_BUFFERS;

    const char *from_str = board_square_to_name(GET_FROM(move));
    const char *to_str = board_square_to_name(GET_TO(move));

    move_str[0] = from_str[0];
    move_str[1] = from_str[1];
    move_str[2] = to_str[0];
    move_str[3] = to_str[1];
    move_str[4] = '\0';

    if (GET_PROMO(move) != 0)
    {
        Piece promo_piece = GET_PROMO(move);

        switch (GET_TYPE(promo_piece))
        {
            case KNIGHT: move_str[4] = 'n'; break;
            case BISHOP: move_str[4] = 'b'; break;
            case ROOK: move_str[4] = 'r'; break;
            case QUEEN: move_str[4] = 'q'; break;
            default: break;
        }

        move_str[5] = '\0';
    }

    return move_str;
}

Move board_parse_move(Game *game, const char *move_str)
{
    if (!game) return (Move){0};

    int from_file = move_str[0] - 'a';
    int from_rank = move_str[1] - '1';
    int to_file   = move_str[2] - 'a';
    int to_rank   = move_str[3] - '1';

    int from_sq = from_rank * 8 + from_file;
    int to_sq   = to_rank * 8 + to_file;

    int is_promo = 0;
    Piece promo_flag = EMPTY;

    if (move_str[4] != '\0')
    {
        is_promo = 1;
        int perspective = isupper(move_str[4]) ? WHITE : BLACK;

        switch (tolower(move_str[4]))
        {
            case 'b': promo_flag = MAKE_PIECE(BISHOP, perspective); break;
            case 'n': promo_flag = MAKE_PIECE(KNIGHT, perspective); break;
            case 'r': promo_flag = MAKE_PIECE(ROOK, perspective); break;
            case 'q': promo_flag = MAKE_PIECE(QUEEN, perspective); break;
            default: promo_flag = EMPTY; break; // Safety fallback
        }
    }

    int capture = 0;
    if (GET_TYPE(game->board_ghost[to_sq]) != EMPTY)
        capture = 1;

    int enpassant = 0;
    if (to_sq == game->enpassant_square)
    {
        enpassant = 1;
        capture = 1; // En passant is still a capture!
    }

    int double_push = 0;
    if (GET_TYPE(board_get_square(game, from_sq)) == PAWN && abs(to_sq - from_sq) == 16)
        double_push = 1;

    int castle = 0;
    if (GET_TYPE(board_get_square(game, from_sq)) == KING && abs(to_sq - from_sq) == 2)
        castle = 1;

    Move move = MOVE(from_sq, to_sq, promo_flag, capture, is_promo, enpassant, 0, double_push, castle);

    return move;
}

int board_has_castling_rights(Game *game, int color)
{
    if (color == WHITE) return (game->castling_rights & (WHITE_KINGSIDE | WHITE_QUEENSIDE)) != 0;
    if (color == BLACK) return (game->castling_rights & (BLACK_KINGSIDE | BLACK_QUEENSIDE)) != 0;

    return 0;
}

int board_has_castling_rights_side(Game *game, int side)
{
    return (game->castling_rights & side) != 0;
}

bool board_is_same_line(int from, int to, int offset) {
    int from_rank = from / 8, from_file = from % 8;
    int to_rank = to / 8, to_file = to % 8;

    switch (offset) {
        case  1: case -1:  // horizontal
            return from_rank == to_rank;

        case  8: case -8:  // vertical
            return from_file == to_file;

        case  9: case -9:  // diagonal (↘ ↖)
            return (to_file - from_file) == (to_rank - from_rank);

        case  7: case -7:  // diagonal (↙ ↗)
            return (to_file - from_file) == -(to_rank - from_rank);

        default:
            return false;
    }
}
