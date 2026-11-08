#include "attacks.h"
#include <algorithm>

namespace Attacks {

Bitboard pawn[2][64], knight[64], king[64];
Magic rookMagics[64], bishopMagics[64];

namespace {
Bitboard rookTable[102400];   // total sizes of all per-square tables (computed offline from the mask bit counts)
Bitboard bishopTable[5248];

uint64_t rngState = 1070372ULL;   // fixed seed -> deterministic magics
uint64_t rnd() {                  // xorshift64*
    rngState ^= rngState >> 12; rngState ^= rngState << 25; rngState ^= rngState >> 27;
    return rngState * 2685821657736338717ULL;
}
uint64_t sparseRnd() { return rnd() & rnd() & rnd(); }   // few set bits -> better magic candidates

const int ROOK_DIRS[4][2]   = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
const int BISHOP_DIRS[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

// Blockers that matter: every square along the rays except the last one (an edge blocker never changes the result).
Bitboard relevantMask(Square s, bool rookDirs) {
    Bitboard a = 0;
    for (auto& d : rookDirs ? ROOK_DIRS : BISHOP_DIRS) {
        int r = rankOf(s) + d[0], f = fileOf(s) + d[1];
        while (r + d[0] >= 0 && r + d[0] < 8 && f + d[1] >= 0 && f + d[1] < 8) {
            a |= bit(r * 8 + f);
            r += d[0]; f += d[1];
        }
    }
    return a;
}

void initMagics(bool rookDirs) {
    Bitboard* table = rookDirs ? rookTable : bishopTable;
    Magic* M = rookDirs ? rookMagics : bishopMagics;
    static Bitboard occ[4096], ref[4096];
    size_t offset = 0;
    for (Square s = 0; s < 64; ++s) {
        Magic& m = M[s];
        m.mask = relevantMask(s, rookDirs);
        int bits = popcount(m.mask);
        m.shift = 64 - bits;
        m.attacks = table + offset;
        int n = 1 << bits;
        // Enumerate every subset of the mask (carry-rippler trick) with its true attack set.
        Bitboard sub = 0; int i = 0;
        do { occ[i] = sub; ref[i] = slidingAttacks(s, sub, rookDirs); ++i; sub = (sub - m.mask) & m.mask; } while (sub);
        for (;;) {
            m.magic = sparseRnd();
            if (popcount((m.mask * m.magic) >> 56) < 6) continue;
            std::fill(m.attacks, m.attacks + n, 0ULL);
            bool ok = true;
            for (i = 0; i < n && ok; ++i) {
                Bitboard& e = m.attacks[(occ[i] * m.magic) >> m.shift];
                if (e == 0) e = ref[i];            // attack sets are never empty, so 0 means "unused"
                else if (e != ref[i]) ok = false;  // destructive collision
            }
            if (ok) break;
        }
        offset += n;
    }
}
}  // namespace

Bitboard slidingAttacks(Square s, Bitboard occ, bool rookDirs) {
    Bitboard a = 0;
    for (auto& d : rookDirs ? ROOK_DIRS : BISHOP_DIRS) {
        int r = rankOf(s) + d[0], f = fileOf(s) + d[1];
        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            Bitboard b = bit(r * 8 + f);
            a |= b;
            if (occ & b) break;
            r += d[0]; f += d[1];
        }
    }
    return a;
}

void init() {
    static bool done = false;
    if (done) return;
    done = true;
    for (Square s = 0; s < 64; ++s) {
        Bitboard b = bit(s);
        pawn[WHITE][s] = northEast(b) | northWest(b);
        pawn[BLACK][s] = southEast(b) | southWest(b);
        knight[s] = northEast(north(b)) | northWest(north(b)) | southEast(south(b)) | southWest(south(b))
                  | east(northEast(b))  | east(southEast(b))  | west(northWest(b))  | west(southWest(b));
        king[s] = north(b) | south(b) | east(b) | west(b) | northEast(b) | northWest(b) | southEast(b) | southWest(b);
    }
    initMagics(true);
    initMagics(false);
}

}  // namespace Attacks
