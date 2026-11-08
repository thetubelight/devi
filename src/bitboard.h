// Set-wise bitboard operations. Everything here is side-effect free (except popLsb) and constexpr/inline.
#pragma once
#include <bit>
#include "types.h"

constexpr Bitboard FILE_A = 0x0101010101010101ULL, FILE_H = FILE_A << 7;
constexpr Bitboard RANK_1 = 0xFFULL, RANK_2 = RANK_1 << 8, RANK_3 = RANK_1 << 16, RANK_4 = RANK_1 << 24,
                   RANK_5 = RANK_1 << 32, RANK_6 = RANK_1 << 40, RANK_7 = RANK_1 << 48, RANK_8 = RANK_1 << 56;

// --- single-square operations (bit twiddling on one square) ---
constexpr Bitboard bit(Square s) { return 1ULL << s; }            // note: 1ULL, never `1 << s`
constexpr bool testBit(Bitboard b, Square s) { return (b >> s) & 1ULL; }
constexpr void setBit(Bitboard& b, Square s) { b |= bit(s); }
constexpr void clearBit(Bitboard& b, Square s) { b &= ~bit(s); }
constexpr void toggleBit(Bitboard& b, Square s) { b ^= bit(s); }

// --- counting / scanning ---
constexpr int  popcount(Bitboard b) { return std::popcount(b); }
constexpr bool moreThanOne(Bitboard b) { return b & (b - 1); }
constexpr Square lsb(Bitboard b) { return std::countr_zero(b); }          // undefined result for b == 0
constexpr Square msb(Bitboard b) { return 63 - std::countl_zero(b); }     // undefined result for b == 0
inline Square popLsb(Bitboard& b) { Square s = lsb(b); b &= b - 1; return s; }

// --- set algebra helpers (named for readability at call sites) ---
constexpr Bitboard setUnion(Bitboard a, Bitboard b) { return a | b; }
constexpr Bitboard setIntersect(Bitboard a, Bitboard b) { return a & b; }
constexpr Bitboard setDiff(Bitboard a, Bitboard b) { return a & ~b; }

// --- directional shifts (file-wrap safe) ---
constexpr Bitboard north(Bitboard b) { return b << 8; }
constexpr Bitboard south(Bitboard b) { return b >> 8; }
constexpr Bitboard east(Bitboard b) { return (b << 1) & ~FILE_A; }
constexpr Bitboard west(Bitboard b) { return (b >> 1) & ~FILE_H; }
constexpr Bitboard northEast(Bitboard b) { return (b << 9) & ~FILE_A; }
constexpr Bitboard northWest(Bitboard b) { return (b << 7) & ~FILE_H; }
constexpr Bitboard southEast(Bitboard b) { return (b >> 7) & ~FILE_A; }
constexpr Bitboard southWest(Bitboard b) { return (b >> 9) & ~FILE_H; }

std::string bitboardToString(Bitboard b);   // debug helper (8x8 grid, rank 8 on top)
