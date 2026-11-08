#include "zobrist.h"

namespace Zobrist {
uint64_t piece[12][64], castling[16], ep[8], side;

void init() {
    uint64_t s = 0x9E3779B97F4A7C15ULL;   // splitmix64
    auto next = [&s]() {
        uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    };
    for (auto& p : piece) for (auto& k : p) k = next();
    for (auto& k : castling) k = next();
    for (auto& k : ep) k = next();
    side = next();
}
}
