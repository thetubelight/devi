# Devi

![CI](https://github.com/thetubelight/devi/actions/workflows/ci.yml/badge.svg)

**Devi** is a UCI chess engine written from scratch in C++20. It is small and readable (about 1,400 lines of source),
verified against known perft counts, and built to be extended: bitboards with magic-bitboard sliders, a negamax / alpha-beta
search with a transposition table, and a deliberately simple evaluation that is easy to replace.

Devi has no board of its own: it speaks the UCI protocol over stdin/stdout, so you play it through a chess GUI
(Arena, Cute Chess, En Croissant, ...) or any other UCI-compatible tool.

## Features

- **Board:** bitboards plus a mailbox mirror, Zobrist hashing, FEN parsing and printing (missing clock fields are tolerated).
- **Move generation:** pseudo-legal moves with a legality check on make; magic bitboards for bishops, rooks and queens,
  generated at startup.
- **Search:** iterative deepening, negamax with alpha-beta, quiescence search, transposition table, principal variation search,
  null-move pruning, late-move reductions, check extensions, and MVV-LVA / killer / history move ordering.
- **Evaluation:** material plus piece-square tables, with a king table tapered between middlegame and endgame.
- **Rules and time:** castling, en passant, promotions, fifty-move rule, repetition and insufficient-material draws;
  clock-based time management (`wtime`/`btime`/increments), `movetime`, `depth`, `nodes` and `infinite`.
- **Tooling:** hashed perft, `bench`, unit tests, SPRT scripts, and CI on Windows and Linux.

Playing strength has not been rated yet. See [Testing and verification](#testing-and-verification) for what has been measured.

## Download

A prebuilt 64-bit Windows binary is attached to each release:
**[Releases](https://github.com/thetubelight/devi/releases)** (`devi-v1.0-win64.exe`, no installer or extra DLLs needed).
Windows SmartScreen may warn about an unsigned program; choose *More info > Run anyway*, or build from source below.

## Build from source (Windows + VS Code)

1. Install **Git**, **VS Code** (with the *C/C++* and *CMake Tools* extensions) and a C++ toolchain:
   **MSYS2 MinGW-w64** (recommended) or **Visual Studio Build Tools**.
2. `git clone https://github.com/thetubelight/devi.git`, then open the folder in VS Code (`code .`).
3. In CMake Tools pick your compiler **kit**, set the build type to **Release**, and click **Build**.
4. Run `build\devi.exe` (MinGW) or `build\Release\devi.exe` (MSVC) and type `uci`.

The full click-by-click guide, with the expected output at each step and a troubleshooting table, is
**[docs/BUILD_WINDOWS.md](docs/BUILD_WINDOWS.md)**.

### From the command line

| Toolchain | Commands (from the repo folder) | Binary |
|---|---|---|
| MSYS2 MinGW-w64 (UCRT64 shell, or PowerShell with `C:\msys64\ucrt64\bin` on PATH) | `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`<br>`cmake --build build` | `build\devi.exe` |
| Visual Studio (Developer PowerShell for VS 2022) | `cmake -S . -B build`<br>`cmake --build build --config Release` | `build\Release\devi.exe` |
| Linux / macOS | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`<br>`cmake --build build -j` | `build/devi` |
| No CMake (MinGW, PowerShell) | `g++ -std=c++20 -O3 -DNDEBUG -static -o devi.exe (Get-ChildItem src\*.cpp).FullName` | `devi.exe` |

Requirements: a C++20 compiler (GCC 10+, Clang 10+, or Visual Studio 2019 16.8+ / 2022) and CMake 3.16+.
Add `-DDEVI_NATIVE=ON` for a build tuned to your CPU; don't share that binary, it may not run on other machines.
`scripts\build.ps1` wraps the Windows steps.

## Using Devi

**In a chess GUI:** add `devi.exe` as a UCI engine.
Arena: *Engines > Install New Engine...* and choose type UCI. Cute Chess: *Tools > Settings > Engines > +*, protocol UCI.

**In a terminal:**

```
> build\devi.exe
uci
position startpos moves e2e4 e7e5
go movetime 1000
```

**UCI options**

| Option | Default | Range | Meaning |
|---|---|---|---|
| `Hash` | 64 | 1-4096 MB | Transposition table size (64-256 is plenty) |
| `Move Overhead` | 30 | 0-5000 ms | Safety margin per move; use ~100 when playing over a network |

**Commands:** `uci`, `isready`, `ucinewgame`, `position [startpos | fen ...] [moves ...]`,
`go [depth | movetime | wtime | btime | winc | binc | movestogo | nodes | infinite]`, `stop`, `setoption`, `quit`.
`position fen` accepts FEN with the halfmove/fullmove fields omitted.
Developer commands: `d` (print the board), `eval`, `perft N`, `divide N`, `bench [depth]`.

## How it works

| File | Role |
|---|---|
| `types.h` | Colors, pieces, squares and the 16-bit move encoding (from, to, flags) |
| `bitboard.h` | Set-wise bitboard operations: set/clear/toggle, shifts, bit scans, popcount |
| `attacks.cpp` | Knight, king and pawn tables; magic-bitboard lookup for sliding pieces |
| `zobrist.cpp` | Random keys for incremental position hashing |
| `position.cpp` | Board state, FEN in/out, make/unmake, attack detection, draw helpers |
| `movegen.cpp` | Pseudo-legal move generation (plus a captures-only mode for quiescence) |
| `perft.cpp` | Perft and divide, with a hash table for speed |
| `eval.cpp` | Material and piece-square-table evaluation |
| `tt.cpp` | Search transposition table |
| `search.cpp` | Iterative deepening, negamax / alpha-beta, quiescence, pruning and ordering |
| `uci.cpp` | UCI loop; the search runs on its own thread so `stop` is always responsive |

Design choices worth knowing:
- **Pseudo-legal generation:** moves are generated without checking king safety; each is made, and unmade if it leaves the
  mover's king attacked. This keeps the generator simple and fast, and perft proves it correct.
- **One source of truth for updates:** `putPiece`, `removePiece` and `movePiece` are the only functions that touch the
  bitboards, the mailbox and the hash key together, so they can't drift apart.
- **Replaceable evaluation:** `evaluate(const Position&)` is the single entry point, returning centipawns from the side to
  move's perspective, so a tuned or neural evaluation can be dropped in later.

## Testing and verification

```powershell
ctest --test-dir build --output-on-failure             # unit tests + perft (depth <= 5) + bench smoke test (MSVC: add -C Release)
build\perft_tests.exe --deep                           # perft depth 6-7 (takes minutes)
.\tests\sprt\run_sprt.ps1 -New .\bin\devi-candidate.exe -Base .\bin\devi-v1.0.exe   # strength regression (needs fastchess)
```

- **Perft:** exact node counts on six standard positions, e.g. start position depth 7 = 3,195,901,860.

  | Position | Depth | Nodes |
  |---|---|---|
  | Start position | 6 | 119,060,324 |
  | Start position | 7 | 3,195,901,860 |
  | Kiwipete | 6 | 8,031,647,685 |
  | Position 3 | 7 | 178,633,661 |

- **Unit tests:** FEN round-trips and rejection of bad input, make/unmake symmetry on random games, Zobrist consistency,
  en passant / castling / promotion edge cases, magic tables against a slow reference, and search sanity checks.
- **Bench fingerprint:** `devi.exe bench 7` prints `1107618 nodes` for v1.0. The count depends only on the search and
  evaluation code, so an unexpected change means behavior changed.
- **SPRT:** adding the transposition table, PVS, null-move pruning and LMR to a plain negamax baseline was accepted at roughly
  +240 Elo (very short time control, so a rough figure). Results are logged in [tests/sprt/README.md](tests/sprt/README.md).
- **CI:** every push builds and tests on Windows (MSVC) and Linux.

Step-by-step instructions, including SPRT setup: **[docs/TESTING.md](docs/TESTING.md)**.

## Roadmap

- **v1.1:** proper repetition handling. Today any single repetition counts as a draw; the plan is the standard approach
  (a repeat inside the search tree is a draw, a repeat of a game-history position needs a true threefold).
- **v1.2:** record the en passant square only when a capture is actually possible (touches FEN, Zobrist and perft).
- **Later:** Lichess bot, aspiration windows, static exchange evaluation, a richer evaluation and tuning, multithreaded search.

## Acknowledgements

- **Sebastian Lague**'s *Coding Adventure: Chess* video, and the code from it
  ([Chess-V1-Unity](https://github.com/SebLague/Chess-Coding-Adventure/tree/Chess-V1-Unity)): which motivated me to pick this project.
- **[Sarah](https://github.com/lostnthought/sarah)** by lostnthought, an engine written in C, studied as a reference for how a
  complete engine fits together.
- The [Chess Programming Wiki](https://www.chessprogramming.org/) was the main technical reference.
- The piece-square tables come from Tomasz Michniewski's *Simplified Evaluation Function*.
- Test openings come from [official-stockfish/books](https://github.com/official-stockfish/books), and strength testing uses
  [fastchess](https://github.com/Disservin/fastchess).

## License

Released under the MIT License. See [LICENSE](LICENSE).