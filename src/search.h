#pragma once
#include <atomic>
#include <cstdint>
#include "position.h"

constexpr int MATE = 31000;      // mate score; "mate in n plies" is MATE - n
constexpr int INF = 32000;
constexpr int MAX_PLY = 128;

struct SearchLimits {
    int      depth = 0;            // 0 = no depth limit
    int64_t  movetime = 0;         // ms, 0 = unset
    int64_t  wtime = -1, btime = -1, winc = 0, binc = 0;   // ms, -1 = unset
    int      movestogo = 0;
    uint64_t nodes = 0;            // 0 = no node limit
    bool     infinite = false;
    int      moveOverhead = 30;    // ms safety margin per move
};

struct SearchResult {
    Move     best = NULL_MOVE;
    int      score = 0;
    int      depth = 0;            // last fully completed depth
    uint64_t nodes = 0;
};

// Runs iterative deepening on a COPY of `pos` state (pos is restored on return). `stop` may be set from another thread.
SearchResult search(Position& pos, const SearchLimits& limits, std::atomic<bool>& stop, bool verbose = true);
