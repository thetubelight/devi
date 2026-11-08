// Precomputed attack tables: leapers (pawn/knight/king) and magic bitboards for sliders.
#pragma once
#include "bitboard.h"

namespace Attacks {

void init();   // must be called once at startup (magics are generated at runtime, takes a few ms)

extern Bitboard pawn[2][64];   // squares attacked by a pawn of the given color standing on the square
extern Bitboard knight[64];
extern Bitboard king[64];

struct Magic {
    Bitboard  mask;      // relevant occupancy squares (board edges excluded)
    Bitboard  magic;
    Bitboard* attacks;
    int       shift;
};
extern Magic rookMagics[64], bishopMagics[64];

inline Bitboard rook(Square s, Bitboard occ) {
    const Magic& m = rookMagics[s];
    return m.attacks[((occ & m.mask) * m.magic) >> m.shift];
}
inline Bitboard bishop(Square s, Bitboard occ) {
    const Magic& m = bishopMagics[s];
    return m.attacks[((occ & m.mask) * m.magic) >> m.shift];
}
inline Bitboard queen(Square s, Bitboard occ) { return rook(s, occ) | bishop(s, occ); }

// Slow reference implementation (ray walking); used to build the tables and in tests.
Bitboard slidingAttacks(Square s, Bitboard occ, bool rookDirs);

}  // namespace Attacks
