// Search transposition table (separate from the perft hash table in perft.h).
#pragma once
#include <vector>
#include "types.h"

enum Bound : uint8_t { BOUND_NONE = 0, BOUND_EXACT = 1, BOUND_LOWER = 2, BOUND_UPPER = 3 };

struct TTEntry {
    uint64_t key;
    Move     move;     // best move found (NULL_MOVE if none/unreliable)
    int16_t  score;    // mate scores are stored relative to the node (see scoreToTT)
    uint8_t  depth;
    uint8_t  bound;
};

class TranspositionTable {
public:
    TranspositionTable() { resize(64); }
    void resize(size_t megabytes);
    void clear();
    const TTEntry* probe(uint64_t key) const {   // nullptr on miss
        const TTEntry& e = table[key & mask];
        return e.key == key && e.bound != BOUND_NONE ? &e : nullptr;
    }
    void store(uint64_t key, int depth, int score, Bound bound, Move move);
private:
    std::vector<TTEntry> table;
    uint64_t mask = 0;
};

extern TranspositionTable TT;
