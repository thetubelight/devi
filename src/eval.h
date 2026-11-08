#pragma once
#include "position.h"

// Static evaluation in centipawns from the SIDE TO MOVE's perspective (what negamax needs).
// This is the single replaceable evaluation entry point (swap for a tuned/NNUE eval later).
int evaluate(const Position& pos);
