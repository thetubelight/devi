#include "movegen.h"

namespace {

void addPromos(MoveList& ml, Square from, Square to, bool capture) {
    int base = capture ? PROMO_CAP_N : PROMO_N;
    ml.add(makeMove(from, to, base + 3));   // queen first (better ordering)
    ml.add(makeMove(from, to, base + 2));
    ml.add(makeMove(from, to, base + 1));
    ml.add(makeMove(from, to, base + 0));
}

inline Bitboard pieceAttacks(PieceType t, Square s, Bitboard occ) {
    switch (t) {
        case KNIGHT: return Attacks::knight[s];
        case BISHOP: return Attacks::bishop(s, occ);
        case ROOK:   return Attacks::rook(s, occ);
        case QUEEN:  return Attacks::queen(s, occ);
        default:     return Attacks::king[s];
    }
}

//compile time specialization
template <bool CAPS_ONLY>
void gen(const Position& p, MoveList& ml) {
    const Color us = p.stm, them = ~us;
    const Bitboard own = p.occ[us], opp = p.occ[them], all = p.occAll;

    // ---- pawns: pushes are set-wise shifts, captures use the attack table ----
    const Bitboard pawns = p.of(us, PAWN);
    const int up = us == WHITE ? 8 : -8;
    const Bitboard promoRank = us == WHITE ? RANK_8 : RANK_1;
    Bitboard one = (us == WHITE ? north(pawns) : south(pawns)) & ~all;
    Bitboard two = (us == WHITE ? north(one & RANK_3) : south(one & RANK_6)) & ~all;

    Bitboard b = one & promoRank;
    while (b) { Square to = popLsb(b); addPromos(ml, to - up, to, false); }
    if (!CAPS_ONLY) {
        b = one & ~promoRank;
        while (b) { Square to = popLsb(b); ml.add(makeMove(to - up, to, QUIET)); }
        b = two;
        while (b) { Square to = popLsb(b); ml.add(makeMove(to - 2 * up, to, DOUBLE_PUSH)); }
    }
    Bitboard pp = pawns;
    while (pp) {
        Square from = popLsb(pp);
        Bitboard att = Attacks::pawn[us][from];
        Bitboard caps = att & opp;
        while (caps) {
            Square to = popLsb(caps);
            if (bit(to) & promoRank) addPromos(ml, from, to, true);
            else ml.add(makeMove(from, to, CAPTURE));
        }
        if (p.ep != NO_SQ && (att & bit(p.ep))) ml.add(makeMove(from, p.ep, EN_PASSANT));
    }

    // ---- knights, bishops, rooks, queens, king ----
    for (PieceType t : {KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
        Bitboard bb = p.of(us, t);
        while (bb) {
            Square from = popLsb(bb);
            Bitboard att = pieceAttacks(t, from, all) & ~own;
            Bitboard caps = att & opp;
            while (caps) ml.add(makeMove(from, popLsb(caps), CAPTURE));
            if (!CAPS_ONLY) {
                Bitboard quiets = att & ~opp;
                while (quiets) ml.add(makeMove(from, popLsb(quiets), QUIET));
            }
        }
    }

    // ---- castling (rights already guarantee king and rook are on their home squares) ----
    if (!CAPS_ONLY) {
        if (us == WHITE) {
            if ((p.castling & WK) && !(all & (bit(F1) | bit(G1))) &&
                !p.attacked(E1, BLACK) && !p.attacked(F1, BLACK) && !p.attacked(G1, BLACK))
                ml.add(makeMove(E1, G1, KING_CASTLE));
            if ((p.castling & WQ) && !(all & (bit(B1) | bit(C1) | bit(D1))) &&
                !p.attacked(E1, BLACK) && !p.attacked(D1, BLACK) && !p.attacked(C1, BLACK))
                ml.add(makeMove(E1, C1, QUEEN_CASTLE));
        } else {
            if ((p.castling & BK) && !(all & (bit(F8) | bit(G8))) &&
                !p.attacked(E8, WHITE) && !p.attacked(F8, WHITE) && !p.attacked(G8, WHITE))
                ml.add(makeMove(E8, G8, KING_CASTLE));
            if ((p.castling & BQ) && !(all & (bit(B8) | bit(C8) | bit(D8))) &&
                !p.attacked(E8, WHITE) && !p.attacked(D8, WHITE) && !p.attacked(C8, WHITE))
                ml.add(makeMove(E8, C8, QUEEN_CASTLE));
        }
    }
}
}  // namespace

void generate(const Position& pos, MoveList& out) { gen<false>(pos, out); }
void generateCaptures(const Position& pos, MoveList& out) { gen<true>(pos, out); }
