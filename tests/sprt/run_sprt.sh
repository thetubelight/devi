#!/usr/bin/env bash
# SPRT: does NEW beat BASE? Wraps fastchess (preferred) or cutechess-cli.
#
# Usage:  tests/sprt/run_sprt.sh <new-binary> <base-binary> [extra tool args...]
# Env:    TC=8+0.08  ELO0=0  ELO1=5  ALPHA=0.05  BETA=0.05  CONCURRENCY=6  ROUNDS=20000
#         HASH=16    BOOK=tests/sprt/openings/book.epd
#
# Early development (big gains expected): ELO0=0 ELO1=10. Later: ELO1=5 or 3.
# Verify flag names against your installed version of the tool (`fastchess --help`).
set -euo pipefail

NEW=${1:?usage: run_sprt.sh <new-binary> <base-binary>}
BASE=${2:?usage: run_sprt.sh <new-binary> <base-binary>}
shift 2

TC=${TC:-8+0.08}
ELO0=${ELO0:-0}
ELO1=${ELO1:-5}
ALPHA=${ALPHA:-0.05}
BETA=${BETA:-0.05}
CONCURRENCY=${CONCURRENCY:-6}   # i5-13450HX has 16 threads; leave headroom so timing isn't distorted
ROUNDS=${ROUNDS:-20000}         # each round = 2 games (colors swapped on the same opening)
HASH=${HASH:-16}
BOOK=${BOOK:-$(dirname "$0")/openings/book.epd}
LOG=${LOG:-sprt_$(date +%Y%m%d_%H%M%S).log}

if command -v fastchess >/dev/null 2>&1; then
  fastchess \
    -engine cmd="$NEW"  name=new  option.Hash="$HASH" \
    -engine cmd="$BASE" name=base option.Hash="$HASH" \
    -each tc="$TC" proto=uci \
    -openings file="$BOOK" format=epd order=random \
    -rounds "$ROUNDS" -games 2 -repeat \
    -concurrency "$CONCURRENCY" -recover \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -ratinginterval 50 "$@" 2>&1 | tee "$LOG"
elif command -v cutechess-cli >/dev/null 2>&1; then
  cutechess-cli \
    -engine cmd="$NEW"  name=new  option.Hash="$HASH" proto=uci \
    -engine cmd="$BASE" name=base option.Hash="$HASH" proto=uci \
    -each tc="$TC" \
    -openings file="$BOOK" format=epd order=random \
    -rounds "$ROUNDS" -games 2 -repeat -concurrency "$CONCURRENCY" -recover \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -ratinginterval 50 "$@" 2>&1 | tee "$LOG"
else
  echo "Neither fastchess nor cutechess-cli found in PATH." >&2
  exit 1
fi
echo "Log saved to $LOG. Append the result to tests/sprt/README.md."
