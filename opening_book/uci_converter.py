import chess.pgn

pgn_file = "games.pgn"
uci_file = "games.uci"
opening_length = 6  # number of moves to consider for opening uniqueness

seen_openings = set()

with open(pgn_file) as f, open(uci_file, "w") as out:
    while True:
        game = chess.pgn.read_game(f)
        if game is None:
            break  # EOF

        board = game.board()
        uci_moves = []
        opening_moves = []

        for i, move in enumerate(game.mainline_moves()):
            uci_moves.append(move.uci())
            board.push(move)
            if i < opening_length:
                opening_moves.append(move.uci())

        opening_key = tuple(opening_moves)

        if opening_key in seen_openings:
            continue  # skip repeated opening

        seen_openings.add(opening_key)

        # Write the UCI moves with separator
        out.write(" ".join(uci_moves) + "\n")
        out.write("---\n")
