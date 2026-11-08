#include "tt.h"
#include <algorithm>

TranspositionTable TT;

void TranspositionTable::resize(size_t megabytes) {
    size_t n = 1;
    while (n * 2 * sizeof(TTEntry) <= megabytes * 1024 * 1024) n *= 2;   // largest power of two that fits
    table.assign(n, TTEntry{0, 0, 0, 0, BOUND_NONE});
    mask = n - 1;
}

void TranspositionTable::clear() { std::fill(table.begin(), table.end(), TTEntry{0, 0, 0, 0, BOUND_NONE}); }

void TranspositionTable::store(uint64_t key, int depth, int score, Bound bound, Move move) {
    TTEntry& e = table[key & mask];
    if (e.key == key && move == NULL_MOVE) move = e.move;   // keep a known best move for this position
    e = TTEntry{key, move, (int16_t)score, (uint8_t)std::min(depth, 255), (uint8_t)bound};
}
