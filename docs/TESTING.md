# Testing Devi

Three layers, from fastest to slowest. Commands are for Windows PowerShell from the repo folder. Replace
`build\devi.exe` with `build\Release\devi.exe` if you built with MSVC (see [BUILD_WINDOWS.md](BUILD_WINDOWS.md)).
On Linux/macOS use `./build/devi`.

## 1. Quick checks (seconds): `ctest`

```powershell
ctest --test-dir build --output-on-failure            # MinGW / Ninja
ctest --test-dir build -C Release --output-on-failure # MSVC (multi-config generator)
```

Expected: `100% tests passed, 0 tests failed out of 3`. The three tests are:

| Test | What it checks |
|---|---|
| `unit_tests` | FEN parsing/printing and rejection of bad FENs, make/unmake symmetry on thousands of random games, Zobrist key consistency, en passant / castling / promotion edge cases, magic-bitboard tables vs a slow reference, search sanity (mate in 1, free queen, K vs K draw) |
| `perft_quick` | Perft node counts on six standard positions up to depth 5 (starts correct move generation) |
| `bench` | Engine starts, searches ten positions and exits cleanly |

VS Code: the status bar's **Run CTest** button does the same.

## 2. Perft: proving move generation (minutes)

Perft counts every legal move sequence to a fixed depth. Any wrong rule (castling, en passant, promotion, pins) changes the number,
so exact matches against known counts are the gold standard for move generation.

```powershell
build\perft_tests.exe --deep                          # all listed depths, including 6 and 7 (several minutes)
```

Or ask the engine directly (uses its hashed perft, 128 MB):

```powershell
"position startpos`nperft 6`n" | build\devi.exe
```

Known-good values:

| Position | d5 | d6 | d7 |
|---|---|---|---|
| startpos | 4,865,609 | 119,060,324 | 3,195,901,860 |
| Kiwipete `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1` | 193,690,690 | 8,031,647,685 | n/a |
| Position 3 `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1` | 674,624 | 11,030,083 | 178,633,661 |

**If a count is wrong**, find the faulty move: run `divide N` for the position and compare against Stockfish's `go perft N`
output move by move (`position fen <FEN>` then `divide 4`). The first root move whose count differs is where to dig;
repeat on the position after that move.

## 3. Benchmark fingerprint

```powershell
build\devi.exe bench 7
```

Prints the total nodes over a fixed set of positions. For Devi v1.0 this is **`1107618 nodes`** (depth 6 gives `505819`).
The count only depends on the search/eval code, not on your machine, so:
- you did **not** touch search, evaluation or move ordering, and the number changed: something is wrong; investigate.
- you **did** change them: a different number is expected; update the fingerprint in this file and in `README.md`.

The `nps` figure printed alongside is machine-dependent and is only for rough speed comparisons (use Release builds).

## 4. Strength testing with SPRT (hours)

SPRT answers: *is the new build really stronger than the old one?* It plays many games between the two and stops as soon as the
statistics are conclusive. Do this for every search/evaluation change before accepting it.

### One-time setup

1. **Get a match runner.** Download **fastchess** from https://github.com/Disservin/fastchess/releases (pick the Windows x86-64 build)
   and unzip it, e.g. to `C:\tools\fastchess\`. (cutechess-cli also works with the script.)
   Check it starts: run the `.exe` you find inside the zip with `--help` in PowerShell.
2. **Keep a baseline binary.** Copy the build you consider "accepted" (the current release) into a `bin` folder:

   ```powershell
   mkdir bin
   copy build\devi.exe bin\devi-v1.0.exe        # MSVC: copy build\Release\devi.exe bin\devi-v1.0.exe
   ```

   `bin\` is git-ignored. Keep these files safe (or attach releases on GitHub).
3. **Power settings.** Plug the laptop in, choose the *Best performance* power mode, and close heavy programs, because CPU
   throttling distorts timed games.

### Each test run

1. Make your change, rebuild (Release!), and copy the new binary: `copy build\devi.exe bin\devi-candidate.exe`
2. Run:

   ```powershell
   .\tests\sprt\run_sprt.ps1 -New .\bin\devi-candidate.exe -Base .\bin\devi-v1.0.exe -Tool C:\tools\fastchess\fastchess.exe
   ```

   (Drop `-Tool` if fastchess is on your PATH. If PowerShell blocks the script:
   `powershell -ExecutionPolicy Bypass -File .\tests\sprt\run_sprt.ps1 ...`.) Defaults: 8+0.08 time control, bounds `[0, 5]`,
   6 parallel games, an opening suite shipped in `tests\sprt\openings\`. Useful switches: `-Elo1 10` (faster test while gains are
   large), `-TC "10+0.1"`, `-Concurrency 8`, `-Hash 64`.
3. Watch the output. Every 50 games it prints the Elo difference and the **LLR**. The test ends when LLR leaves the interval
   shown (about -2.94 to +2.94):
   - `H1 was accepted`: the new build is stronger (by at least the upper bound). Keep it, make it the new baseline.
   - `H0 was accepted`: not demonstrably better. Revert or rethink the change.
4. A log file `sprt_<date>_<time>.log` is written to the folder you ran from. Add a row to the results table in
   `tests\sprt\README.md` (date, versions, time control, bounds, W/L/D, Elo, outcome).

**Quick pipeline check** (about a minute): `.\tests\sprt\run_sprt.ps1 -New .\bin\devi-v1.0.exe -Base .\bin\devi-v1.0.exe -TC "2+0.02" -Rounds 4 -Concurrency 1`
plays a few identical-engine games just to prove everything is wired up. Don't read anything into its result.

### Reading the numbers honestly
- Elo from short time controls on one machine is rough and not comparable to rating lists. It's for *relative* decisions between your own builds.
- A single run that ends quickly with a big Elo gain is believable; one that wanders near zero for thousands of games means "no meaningful difference".
- Use a larger opening book for long runs: e.g. `8mvs_big_+80_+109.epd` from https://github.com/official-stockfish/books, passed with `-Book <path>`.