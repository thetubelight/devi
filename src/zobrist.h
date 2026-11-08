#pragma once
#include "types.h"

namespace Zobrist {
extern uint64_t piece[12][64];
extern uint64_t castling[16];
extern uint64_t ep[8];      // by file
extern uint64_t side;       // XORed in when black is to move
void init();
}
