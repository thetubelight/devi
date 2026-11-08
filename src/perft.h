#pragma once
#include <vector>
#include "position.h"

// Perft transposition table: maps (zobrist key, depth) -> node count.
// Depth is folded into the stored key so a hit can never return a count for the wrong depth.
class PerftTT {
public:
    explicit PerftTT(size_t megabytes = 128);
    bool probe(uint64_t key, int depth, uint64_t& nodes) const;
    void store(uint64_t key, int depth, uint64_t nodes);
    void clear();
private:
    struct Entry { uint64_t key; uint64_t nodes; };
    std::vector<Entry> table;
    uint64_t mask;
    static uint64_t mix(uint64_t key, int depth) { return key ^ (0x9E3779B97F4A7C15ULL * (uint64_t)(depth + 1)); }
};

uint64_t perft(Position& pos, int depth, PerftTT* tt = nullptr);   // bulk-counts at depth 1; hashes at depth >= 2
uint64_t divide(Position& pos, int depth, PerftTT* tt = nullptr);  // prints per-root-move counts, returns total
std::string moveToString(Move m);                                  // long algebraic ("e2e4", "e7e8q")
