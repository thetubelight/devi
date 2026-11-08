#pragma once
#include "position.h"

struct MoveList {
    Move moves[256];
    int  count = 0;
    void add(Move m) { moves[count++] = m; }
    Move* begin() { return moves; }
    Move* end() { return moves + count; }
    const Move* begin() const { return moves; }
    const Move* end() const { return moves + count; }
};

// Pseudo-legal: may leave the mover's king in check (filter with Position::makeIfLegal).
// Castling is the exception: it is only generated if the king doesn't start on, cross, or land on an attacked square.
void generate(const Position& pos, MoveList& out);
// Captures, en passant and promotions only (for quiescence search).
void generateCaptures(const Position& pos, MoveList& out);
