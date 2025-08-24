#include <table/repetition.h>
#include <table/zobrist.h>

#include <board/attack.h>
#include <board/board.h>

#include <table/magic.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void board_array_from_bitboards(Game *game)
{
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

char* board_generate_fen(Game *game)
{
    if (!game) return NULL;

    // Buffer to hold the FEN string (max length should be 256)
    static char fen_string[256];
    int index = 0;

    // 1. Generate piece placement
    for (int rank = 7; rank >= 0; rank--) // Iterate from top (rank 8) to bottom (rank 1)
    {
        int empty_count = 0;

        for (int file = 0; file < 8; file++) // Iterate from left (file A) to right (file H)
        {
            int square = rank * 8 + file;
            Piece piece = board_get_square(game, square); // Get the piece on this square

            if (GET_TYPE(piece) == EMPTY)
            {
                empty_count++;
            }
            else
            {
                if (empty_count > 0)
                {
                    // Add empty squares count to FEN string
                    fen_string[index++] = '0' + empty_count;
                    empty_count = 0;
                }

                // Append the piece (uppercase for white, lowercase for black)
                char piece_char;
                switch (GET_TYPE(piece))
                {
                    case PAWN:   piece_char = (GET_COLOR(piece) == WHITE) ? 'P' : 'p'; break;
                    case KNIGHT: piece_char = (GET_COLOR(piece) == WHITE) ? 'N' : 'n'; break;
                    case BISHOP: piece_char = (GET_COLOR(piece) == WHITE) ? 'B' : 'b'; break;
                    case ROOK:   piece_char = (GET_COLOR(piece) == WHITE) ? 'R' : 'r'; break;
                    case QUEEN:  piece_char = (GET_COLOR(piece) == WHITE) ? 'Q' : 'q'; break;
                    case KING:   piece_char = (GET_COLOR(piece) == WHITE) ? 'K' : 'k'; break;
                    default:     piece_char = ' '; break;
                }
                fen_string[index++] = piece_char;
            }
        }

        // If there are empty squares at the end of the rank, add the count
        if (empty_count > 0)
        {
            fen_string[index++] = '0' + empty_count;
        }

        if (rank > 0)
        {
            fen_string[index++] = '/'; // Separator between ranks
        }
    }

    fen_string[index++] = ' ';

    // 2. Add turn (white or black)
    fen_string[index++] = (game->turn == WHITE) ? 'w' : 'b';
    fen_string[index++] = ' ';

    // 3. Add castling rights
    if (game->castling_rights == 0)
    {
        fen_string[index++] = '-';
    }
    else
    {
        if (game->castling_rights & 0x1) fen_string[index++] = 'K'; // White kingside
        if (game->castling_rights & 0x2) fen_string[index++] = 'Q'; // White queenside
        if (game->castling_rights & 0x4) fen_string[index++] = 'k'; // Black kingside
        if (game->castling_rights & 0x8) fen_string[index++] = 'q'; // Black queenside
    }
    fen_string[index++] = ' ';

    // 4. Add en passant, halfmove clock, and fullmove number (optional, can be skipped for now)
    // We'll leave it blank for now, as this data isn't specified.
    fen_string[index++] = '-';
    fen_string[index++] = ' ';
    fen_string[index++] = '0'; // Halfmove clock
    fen_string[index++] = ' ';
    fen_string[index++] = '1'; // Fullmove number

    // Null-terminate the FEN string
    fen_string[index] = '\0';

    return fen_string;
}

void board_load_fen(Game *game, const char *fen_string)
{
    if (!game || !fen_string) return;

    if (!game->board_is_first_load) game->board_is_first_load = 1;

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
    if (game->board_is_first_load)
    {
        attack_generate_table(game, WHITE);
        attack_generate_table(game, BLACK);
        zobrist_update_board(game);
    }
}

char *board_get_checkers(Game *game)
{
    static char buffer[64]; // stores piece positions as strings like "e4 h5"
    buffer[0] = '\0';       // start empty

    int us   = game->turn;
    int them = us ^ 1;

    // Find our king
    Bitboard king_bb = game->board[us][KING];
    if (!king_bb) return buffer; // no king (illegal position)

    int king_sq = __builtin_ctzll(king_bb);

    // Current occupancy of all pieces
    Bitboard occupancy = game->occupancy[BOTH];

    // Bitboard of checking pieces
    Bitboard checkers_bb = 0ULL;

    for (int pt = PAWN; pt <= KING; pt++) {
        Bitboard pieces = game->board[them][pt];
        while (pieces) {
            int sq = __builtin_ctzll(pieces);
            pieces &= pieces - 1;

            Bitboard attacks = 0ULL;

            switch (pt) {
                case PAWN:
                    attacks = game->attack_tables_pc_pawn[them][sq]; // color-aware pawn attack mask
                    break;

                case KNIGHT:
                    attacks = game->attack_tables_pc[KNIGHT][sq];
                    break;

                case BISHOP:
                    attacks = magic_get_bishop_attacks(sq, occupancy);
                    break;

                case ROOK:
                    attacks = magic_get_rook_attacks(sq, occupancy);
                    break;

                case QUEEN:
                    attacks = magic_get_bishop_attacks(sq, occupancy) | magic_get_rook_attacks(sq, occupancy);
                    break;

                case KING:
                    attacks = game->attack_tables_pc[KING][sq];
                    break;
            }

            if (attacks & (1ULL << king_sq)) {
                checkers_bb |= (1ULL << sq);
            }
        }
    }

    // Convert checkers bitboard to space-separated string
    while (checkers_bb) {
        int sq = __builtin_ctzll(checkers_bb);
        checkers_bb &= checkers_bb - 1;

        char file = 'a' + (sq % 8);
        char rank = '1' + (sq / 8);
        size_t len = strlen(buffer);
        sprintf(buffer + len, "%c%c ", file, rank);
    }

    return buffer;
}

void board_print(Game *game)
{
    if (!game) return;

    const char piece_chars[7] = {'.', 'P', 'N', 'B', 'R', 'Q', 'K'};

    printf("\n");

    for (int rank = 7; rank >= 0; rank--)
    {
        printf(" +---+---+---+---+---+---+---+---+\n ");

        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;

            Piece p = board_get_square(game, square);

            if (p == 0)
            {
                printf("|   ");
            }
            else
            {
                int type = p & 0x07;
                int color = (p & 0x08) >> 3;

                char c = piece_chars[type];
                if (color == 1) c = tolower(c);

                printf("| %c ", c);
            }
        }

        printf("| %d  ", rank + 1);

        printf("\n");
    }

    printf(" +---+---+---+---+---+---+---+---+\n ");

    printf("  a   b   c   d   e   f   g   h \n");

    printf("\nFEN: %s\n", board_generate_fen(game));
    printf("Zobrist Key: %s\n", zobrist_key_to_string(game->zobrist_key));
    printf("Checkers: %s\n", board_get_checkers(game));
    printf("\n");
}

void board_make_move(Game *game, Move move, int generation_type)
{
    if (!game) return;

    const int from = GET_FROM(move);
    const int to   = GET_TO(move);

    const Piece piece = board_get_square(game, from);
    const int color   = GET_COLOR(piece);

    const bool is_ep        = IS_ENPASSANT(move);
    const int ep_capture_sq = (color == WHITE) ? to - 8 : to + 8;
    const Piece captured    = board_get_square(game, is_ep ? ep_capture_sq : to);

    // Save state
    State *s = &game->history[game->history_count++];
    s->castling_rights  = game->castling_rights;
    s->enpassant_square = game->enpassant_square;
    s->captured_piece   = captured;
    s->move             = move;
    if (generation_type == MAKE_MOVE_FULL) s->zobrist_key      = game->zobrist_key;
    s->turn             = game->turn;

    memcpy(s->attack_map, game->attack_map, sizeof(game->attack_map));
    memcpy(s->attack_map_full, game->attack_map_full, sizeof(game->attack_map_full));

    // Apply move
    board_set_square(game, from, EMPTY);
    if (IS_PROMO(move)) {
        board_set_square(game, to, MAKE_PIECE(GET_PROMO(move), color));
    } else {
        board_set_square(game, to, piece);
    }

    if (is_ep) board_set_square(game, ep_capture_sq, EMPTY);

    // Castling — keep branch logic to preserve correctness
    if (IS_CASTLE(move)) {
        int rook_from = 0, rook_to = 0;

        if (from == 4 && to == 6) {         // White kingside
            rook_from = 7;  rook_to = 5;
        } else if (from == 60 && to == 62) { // Black kingside
            rook_from = 63; rook_to = 61;
        } else if (from == 4 && to == 2) {   // White queenside
            rook_from = 0;  rook_to = 3;
        } else if (from == 60 && to == 58) { // Black queenside
            rook_from = 56; rook_to = 59;
        }

        Piece rook = board_get_square(game, rook_from);
        board_set_square(game, rook_from, EMPTY);
        board_set_square(game, rook_to, rook);
    }

    // Update castling rights
    game->castling_rights &= game->castling_rights_lookup[from][to];

    // Update en passant square
    game->enpassant_square = IS_DOUBLE_PUSH(move) ? (color == WHITE ? to - 8 : to + 8) : -1;

    // Incremental attack update
    attack_update_incremental(game, move);

    // Switch turn
    game->turn ^= 1;

    // Zobrist update
    if (generation_type == MAKE_MOVE_FULL) zobrist_update_move(game, move, s);

    // Push repetition key
    if (generation_type == MAKE_MOVE_FULL) repetition_push(game, game->zobrist_key);
}

void board_unmake_move(Game *game, int generation_type)
{
    if (!game) return;
    if (game->history_count <= 0) {
        fprintf(stderr, "Error: unmake_move with empty history!\n");
        exit(EXIT_FAILURE);
    }

    State *s = &game->history[--game->history_count];

    Move move = s->move;
    const int from = GET_FROM(move);
    const int to = GET_TO(move);

    Piece piece = board_get_square(game, to);
    int color = GET_COLOR(piece);

    // Undo turn flip first (since make_move flips at the end)
    game->turn = s->turn;

    // Move piece back from `to` to `from`
    board_set_square(game, to, EMPTY);

    if (IS_PROMO(move)) {
        // On unmake, remove promoted piece at `to`, restore pawn at `from`
        board_set_square(game, from, MAKE_PIECE(PAWN, color));
    } else {
        board_set_square(game, from, piece);
    }

    // Restore captured piece
    if (IS_ENPASSANT(move)) {
        // Restore the captured pawn behind the `to` square
        const int ep_capture_sq = (color == WHITE) ? to - 8 : to + 8;
        board_set_square(game, ep_capture_sq, s->captured_piece);
    } else if (s->captured_piece != EMPTY) {
        board_set_square(game, to, s->captured_piece);
    }

    // Undo castling rook move if castling
    if (IS_CASTLE(move)) {
        int rook_from, rook_to;
        if (to == from + 2) {  // Kingside castle
            rook_from = from + 3;
            rook_to = from + 1;
        } else {               // Queenside castle
            rook_from = from - 4;
            rook_to = from - 1;
        }
        Piece rook = board_get_square(game, rook_to);
        board_set_square(game, rook_to, EMPTY);
        board_set_square(game, rook_from, rook);
    }

    // Restore castling rights, en passant, zobrist key, and attack tables
    game->castling_rights = s->castling_rights;
    game->enpassant_square = s->enpassant_square;
    if (generation_type == MAKE_MOVE_FULL) game->zobrist_key = s->zobrist_key;

    memcpy(game->attack_map, s->attack_map, sizeof(game->attack_map));
    memcpy(game->attack_map_full, s->attack_map_full, sizeof(game->attack_map_full));

    if (generation_type == MAKE_MOVE_FULL) repetition_pop(game);
}

void board_make_null_move(Game *game)
{
    if (!game) return;

    // Save state
    State *s = &game->history[game->history_count++];
    s->castling_rights  = game->castling_rights;
    s->enpassant_square = game->enpassant_square;
    s->zobrist_key      = game->zobrist_key;
    s->turn             = game->turn;

    // Update en passant square
    game->enpassant_square = -1;

    // Switch turn
    game->turn ^= 1;

    // Zobrist update
    zobrist_update_move(game, 0, s);
}

void board_unmake_null_move(Game *game)
{
    if (!game) return;
    if (game->history_count <= 0) {
        fprintf(stderr, "Error: unmake_move with empty history!\n");
        exit(EXIT_FAILURE);
    }

    State *s = &game->history[--game->history_count];

    // Undo turn flip first (since make_move flips at the end)
    game->turn = s->turn;

    game->enpassant_square = s->enpassant_square;
    game->zobrist_key = s->zobrist_key;
}

inline int board_is_on_rank(int square, int rank)
{
    // Assuming square is always valid; remove bounds check for speed
    return ((square >> 3) == rank);
}

inline int board_find_king(const Game *game, int color)
{
    // Assuming game->board[color][KING] always valid
    uint64_t king_bb = game->board[color][KING];
    return king_bb ? __builtin_ctzll(king_bb) : -1;
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
    if (to_sq == game->enpassant_square && GET_TYPE(game->board_ghost[from_sq]) == PAWN)
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

inline int board_has_castling_rights(Game *game, int color)
{
    // Predefined masks indexed by color (assuming WHITE=0, BLACK=1)
    static const int castling_masks[2] = {
        WHITE_KINGSIDE | WHITE_QUEENSIDE,
        BLACK_KINGSIDE | BLACK_QUEENSIDE
    };

    if (color < 0 || color > 1) return 0; // safety check

    return (game->castling_rights & castling_masks[color]) != 0;
}

inline int board_has_castling_rights_side(Game *game, int side)
{
    return (game->castling_rights & side) != 0;
}

inline bool board_is_same_line(int from, int to, int offset)
{
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

inline bool board_move_gives_check(Game *game, Move move)
{
    if (!game) return false;

    board_make_move(game, move, MAKE_MOVE_LIGHT);

    bool check = board_is_king_in_check(game, game->turn);

    board_unmake_move(game, MAKE_MOVE_LIGHT);

    return check;
}

inline bool board_is_same_ray(int square_a, int square_b)
{
    int rank_a = square_a / 8;
    int rank_b = square_b / 8;

    int file_a = square_a % 8;
    int file_b = square_b % 8;
    
    return (rank_a == rank_b || file_a == file_b || abs(rank_a - rank_b) == abs(file_a - file_b));
}

Bitboard board_get_sliding_pieces_bitboard(Game *game, int color)
{
    return game->board[color][BISHOP] | game->board[color][ROOK] | game->board[color][QUEEN];
}