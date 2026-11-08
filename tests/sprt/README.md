# SPRT testing

**Windows (PowerShell):** `.\tests\sprt\run_sprt.ps1 -New .\bin\devi-candidate.exe -Base .\bin\devi-v1.0.exe` (see the script header for options;
`-Tool C:\path\fastchess.exe` if it isn't on PATH). **Linux/macOS/Git Bash:** `tests/sprt/run_sprt.sh <new> <base>`.
Get fastchess for Windows from https://github.com/Disservin/fastchess/releases. Plug the laptop in and use the High performance power plan;
keep concurrency at 6-8 so timing isn't distorted.

`run_sprt` plays NEW against BASE with paired colors on a shared opening suite and stops when the
sequential probability ratio test accepts (NEW is at least `ELO1` stronger) or rejects (no better than `ELO0`).

## Procedure
1. Tag each accepted build and keep its binary: `bin\devi-v1.0.exe` now, `bin\devi-v1.1.exe` after the next accepted release, and so on (`bin/` is git-ignored; keep them somewhere safe, or attach them to GitHub Releases).
2. Build the candidate, run the script with the candidate as NEW and the last accepted build as BASE.
3. Only merge if H1 is accepted. Append the run to the log below.

Defaults: `TC=8+0.08`, bounds `[0, 5]`, alpha = beta = 0.05, `CONCURRENCY=6`.
Early on, use `ELO1=10` (gains are large and the test ends faster). Leave CPU headroom so timing isn't distorted.

## Openings
`openings/book.epd` is the 635-position `4mvs_+90_+99` suite from
[official-stockfish/books](https://github.com/official-stockfish/books). For long runs use a larger one
(e.g. `8mvs_big_+80_+109.epd`, ~26k positions) via `BOOK=/path/to/file.epd`.

## Tools
`fastchess` (https://github.com/Disservin/fastchess) preferred; `cutechess-cli` also supported by the script.

## Results log
| Date | New | Base | TC | Bounds | Games (W/L/D) | Elo | Outcome |
|---|---|---|---|---|---|---|---|
| 2026-11-07 | Devi v1.0 (TT, PVS, null-move, LMR) | pre-release baseline (unreleased: plain negamax + alpha-beta + qsearch, MVV-LVA/killers/history) | 1+0.01, Hash 16 (new only), 1 shared cloud core | [0, 10], a=b=0.05 | 192 (132/17/43) | +240 ± 49 | **H1 accepted** (LLR 2.95) |

Notes: very short TC on a single shared core, so the Elo figure is rough and not comparable to rating lists; the test's purpose here was to validate the pipeline. `bench 7` nodes: pre-release baseline = 39,883,616; Devi v1.0 = 1,107,618.
