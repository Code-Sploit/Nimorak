import streamlit as st
import pandas as pd
import altair as alt
import math
import re
import chess
import chess.pgn
import chess.svg
import plotly.graph_objects as go
import subprocess
import psutil
from pathlib import Path
from io import StringIO
from streamlit_autorefresh import st_autorefresh

# -------------------------
# Files & globals
# -------------------------
PGN_FILE = Path("results.pgn")
PID_FILE = Path("cutechess_pid.txt")
PROCESS = None
ELO_AXIS_RANGE = 100
NUM_BOARDS = 4
BOARD_SIZE = 200  # SVG board size

# -------------------------
# Helpers: parse PGN into Game objects
# -------------------------
def parse_games_from_pgn(path: Path):
    games = []
    if not path.exists():
        return games
    with path.open(encoding="utf-8", errors="ignore") as f:
        while True:
            try:
                g = chess.pgn.read_game(f)
            except Exception:
                break
            if g is None:
                break
            games.append(g)
    return games

# -------------------------
# Normalize result string
# -------------------------
def normalize_result(res):
    if not res:
        return "*"
    if res in ("1-0", "0-1"):
        return res
    if res in ("1/2-1/2", "1/2 - 1/2", "½-½", "1/2–1/2"):
        return "1/2-1/2"
    return res

# -------------------------
# Compute wins/draws robustly
# -------------------------
def compute_stats_games(games, eng1, eng2):
    wins1 = wins2 = draws = 0
    for g in games:
        res = normalize_result(g.headers.get("Result", ""))
        white = g.headers.get("White", "")
        black = g.headers.get("Black", "")
        if res == "1-0":
            if white == eng1:
                wins1 += 1
            elif white == eng2:
                wins2 += 1
        elif res == "0-1":
            if black == eng1:
                wins1 += 1
            elif black == eng2:
                wins2 += 1
        elif res == "1/2-1/2":
            draws += 1
    return wins1, wins2, draws

# -------------------------
# Elo / LOS calculation
# -------------------------
def compute_elo_stats(games, eng1, eng2):
    total = len(games)
    if total == 0:
        return 0.0, 0.0, 50.0
    wins1, wins2, draws = compute_stats_games(games, eng1, eng2)
    score = (wins1 + 0.5 * draws) / total
    if score <= 0:
        return -float("inf"), 0.0, 0.0
    if score >= 1:
        return float("inf"), 0.0, 100.0
    elo = -400.0 * math.log10(1.0 / score - 1.0)
    sigma = (400.0 / math.log(10)) * math.sqrt(score * (1.0 - score) / total)
    los = 100.0 * (0.5 * (1.0 + math.erf(elo / (math.sqrt(2.0) * sigma)))) if sigma > 0 else 50.0
    return elo, sigma, los

def elo_over_time_df(games, eng1, eng2):
    rows = []
    for i in range(1, len(games) + 1):
        prefix = games[:i]
        elo, sigma, los = compute_elo_stats(prefix, eng1, eng2)
        if math.isinf(elo):
            elo_val = None
        else:
            elo_val = elo
        rows.append({"Game": i, "Elo": elo_val, "Sigma": sigma})
    if not rows:
        return pd.DataFrame(columns=["Game", "Elo", "Sigma"])
    df = pd.DataFrame(rows)
    return df

# -------------------------
# Start/Stop cutechess process management
# -------------------------
def start_cutechess(engine1, engine2, num_games):
    global PROCESS
    if PROCESS is None or PROCESS.poll() is not None:
        cmd = f".\\misc\\sprt_test.bat {engine1} {engine2} {num_games}"
        PROCESS = subprocess.Popen(cmd, shell=True, creationflags=0x00000200)
        try:
            PID_FILE.write_text(str(PROCESS.pid))
        except Exception:
            pass

def kill_process_tree(pid):
    try:
        parent = psutil.Process(pid)
        for child in parent.children(recursive=True):
            try:
                child.kill()
            except Exception:
                pass
        parent.kill()
    except Exception:
        pass

def stop_cutechess():
    global PROCESS
    if PROCESS is not None and PROCESS.poll() is None:
        try:
            kill_process_tree(PROCESS.pid)
        except Exception:
            pass
        PROCESS = None
    if PID_FILE.exists():
        try:
            pid = int(PID_FILE.read_text())
            kill_process_tree(pid)
        except Exception:
            pass
        try:
            PID_FILE.unlink()
        except Exception:
            pass

# -------------------------
# Render last legal position of a Game (safely)
# -------------------------
def render_game_svg(game, size=200):
    board = game.board()
    for mv in game.mainline_moves():
        try:
            board.push(mv)
        except Exception:
            break
    svg = chess.svg.board(board=board, size=size)
    return svg, len(board.move_stack)

# -------------------------
# Small gauge helper
# -------------------------
def small_gauge(title, value, max_val=100, height=110):
    fig = go.Figure(go.Indicator(
        mode="gauge+number",
        value=value if value is not None and not (isinstance(value, float) and math.isinf(value)) else 0,
        domain={'x': [0, 1], 'y': [0, 1]},
        gauge={
            'axis': {'range': [0, max_val]},
            'bar': {'color': 'royalblue'},
            'bgcolor': 'lightgray',
        },
        title={'text': title, 'font': {'size': 11}}
    ))
    fig.update_layout(margin=dict(l=6, r=6, t=22, b=6), height=height)
    return fig

# -------------------------
# Detect available engines
# -------------------------
def detect_rune_engines():
    # keep full filenames, not .stem
    exe_files = Path(".").glob("rune_*.exe")
    return [f.name for f in exe_files]

# -------------------------
# Session state defaults
# -------------------------
if "console_lines" not in st.session_state:
    st.session_state.console_lines = []
if "selected_engines" not in st.session_state:
    st.session_state.selected_engines = ("Engine1", "Engine2")
if "board_move_counts" not in st.session_state:
    st.session_state.board_move_counts = [0] * NUM_BOARDS

# -------------------------
# Page config + UI controls
# -------------------------
st.set_page_config("SPRT Dashboard", layout="wide")
st.title("♟️ Live SPRT Dashboard")

with st.sidebar:
    st.subheader("⚙️ Controls")

    detected_runes = detect_rune_engines()
    available_engines = detected_runes + ["stockfish", "leela"]

    engine1_name = st.selectbox("Engine 1", available_engines, index=0 if available_engines else None)
    engine2_name = st.selectbox("Engine 2", available_engines, index=1 if len(available_engines) > 1 else 0)
    num_games = st.number_input("Number of games", min_value=1, max_value=2000, value=100)

    col_start, col_stop = st.columns(2)
    if col_start.button("▶️ Start"):
        try:
            PGN_FILE.write_text("")
        except Exception:
            pass
        st.session_state.console_lines = []
        st.session_state.selected_engines = (engine1_name, engine2_name)
        st.session_state.board_move_counts = [0] * NUM_BOARDS
        start_cutechess(engine1_name, engine2_name, num_games)
    if col_stop.button("⏹ Stop"):
        stop_cutechess()

# auto-refresh
st_autorefresh(interval=2000, key="refresh")

# -------------------------
# Main: parse games and render
# -------------------------
games = parse_games_from_pgn(PGN_FILE)
if len(games) == 0:
    st.warning("No games found in results.pgn yet. Waiting for first game...")
    st.stop()

eng1, eng2 = st.session_state.selected_engines
if eng1 == "Engine1" and eng2 == "Engine2":
    try:
        g0 = games[0]
        eng1 = g0.headers.get("White", eng1)
        eng2 = g0.headers.get("Black", eng2)
        st.session_state.selected_engines = (eng1, eng2)
    except Exception:
        pass

wins1, wins2, draws = compute_stats_games(games, eng1, eng2)
elo, sigma, los = compute_elo_stats(games, eng1, eng2)

col_summary, col_elo, col_stats = st.columns([1.2, 3.0, 0.9])

with col_summary:
    st.subheader("📊 Match Summary")
    st.metric("Games played", len(games))
    st.metric(f"{eng1} wins", wins1)
    st.metric(f"{eng2} wins", wins2)
    st.metric("Draws", draws)

    with st.expander("🖥 Console Output (latest games)", expanded=False):
        lines = []
        for idx, g in enumerate(games, start=1):
            res = normalize_result(g.headers.get("Result", ""))
            term = g.headers.get("Termination", "-")
            white = g.headers.get("White", "-")
            black = g.headers.get("Black", "-")
            lines.append(f"Game {idx}: {white} vs {black} → {res} ({term})")
        st.text("\n".join(lines[-50:]))

with col_elo:
    st.subheader("📈 Elo Δ Over Time")
    df_elo = elo_over_time_df(games, eng1, eng2)
    if not df_elo.empty:
        df_plot = df_elo.copy()
        df_plot["Elo_plot"] = df_plot["Elo"].astype(float).clip(-ELO_AXIS_RANGE, ELO_AXIS_RANGE)
        y_scale = alt.Scale(domain=[-ELO_AXIS_RANGE, ELO_AXIS_RANGE])
        line = alt.Chart(df_plot).mark_line(color="blue").encode(
            x=alt.X("Game:Q"),
            y=alt.Y("Elo_plot:Q", scale=y_scale, title="Elo Δ")
        )
        st.altair_chart(line.properties(height=300), use_container_width=True)
    else:
        st.write("Not enough data for Elo chart yet.")

with col_stats:
    st.subheader("📌 Live Stats")
    st.plotly_chart(small_gauge("Elo Δ", round(elo, 1) if (elo is not None and not math.isinf(elo)) else 0, 100), use_container_width=True)
    st.plotly_chart(small_gauge("Uncertainty σ", round(sigma, 1), 50), use_container_width=True)
    st.plotly_chart(small_gauge("LOS %", round(los, 1), 100), use_container_width=True)

st.subheader("🎮 Latest Boards (last full games)")
latest_games = games[-NUM_BOARDS:]
cols_boards = st.columns(NUM_BOARDS)
for idx, (col, game_obj) in enumerate(zip(cols_boards, latest_games)):
    with col:
        header_white = game_obj.headers.get("White", "?")
        header_black = game_obj.headers.get("Black", "?")
        st.markdown(f"**{header_white}** vs **{header_black}**")
        svg, move_count = render_game_svg(game_obj, size=BOARD_SIZE)
        st.components.v1.html(svg, height=BOARD_SIZE + 20, scrolling=False)

st.write("")
st.caption("Dashboard auto-refreshes every 2s. Start resets results.pgn. Stop kills the running cutechess process (if started from this UI).")
