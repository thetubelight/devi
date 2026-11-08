#!/usr/bin/env bash
# Local release build. Pass -DDEVI_NATIVE=ON for a CPU-specific (faster, non-portable) binary.
set -euo pipefail
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build build -j
