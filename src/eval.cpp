// Material + piece-square tables (Tomasz Michniewski's "Simplified Evaluation Function"),
// with the king table tapered between middlegame and endgame by remaining non-pawn material.
#include "eval.h"

namespace {
constexpr int VALUE[6] = {100, 320, 330, 500, 900, 0};

// Tables are written as printed on a board: first row = rank 8, from White's point of view.
constexpr int PST[6][64] = {
    {  0,  0,  0,  0,  0,  0,  0,  0,   // pawn
      50, 50, 50, 50, 50, 50, 50, 50,
      10, 10, 20, 30, 30, 20, 10, 10,
       5,  5, 10, 25, 25, 10,  5,  5,
       0,  0,  0, 20, 20,  0,  0,  0,
       5, -5,-10,  0,  0,-10, -5,  5,
       5, 10, 10,-20,-20, 10, 10,  5,
       0,  0,  0,  0,  0,  0,  0,  0},
    {-50,-40,-30,-30,-30,-30,-40,-50,   // knight
     -40,-20,  0,  0,  0,  0,-20,-40,
     -30,  0, 10, 15, 15, 10,  0,-30,
     -30,  5, 15, 20, 20, 15,  5,-30,
     -30,  0, 15, 20, 20, 15,  0,-30,
     -30,  5, 10, 15, 15, 10,  5,-30,
     -40,-20,  0,  5,  5,  0,-20,-40,
     -50,-40,-30,-30,-30,-30,-40,-50},
    {-20,-10,-10,-10,-10,-10,-10,-20,   // bishop
     -10,  0,  0,  0,  0,  0,  0,-10,
     -10,  0,  5, 10, 10,  5,  0,-10,
     -10,  5,  5, 10, 10,  5,  5,-10,
     -10,  0, 10, 10, 10, 10,  0,-10,
     -10, 10, 10, 10, 10, 10, 10,-10,
     -10,  5,  0,  0,  0,  0,  5,-10,
     -20,-10,-10,-10,-10,-10,-10,-20},
    {  0,  0,  0,  0,  0,  0,  0,  0,   // rook
       5, 10, 10, 10, 10, 10, 10,  5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
       0,  0,  0,  5,  5,  0,  0,  0},
    {-20,-10,-10, -5, -5,-10,-10,-20,   // queen
     -10,  0,  0,  0,  0,  0,  0,-10,
     -10,  0,  5,  5,  5,  5,  0,-10,
      -5,  0,  5,  5,  5,  5,  0, -5,
       0,  0,  5,  5,  5,  5,  0, -5,
     -10,  5,  5,  5,  5,  5,  0,-10,
     -10,  0,  5,  0,  0,  0,  0,-10,
     -20,-10,-10, -5, -5,-10,-10,-20},
    {-30,-40,-40,-50,-50,-40,-40,-30,   // king, middlegame
     -30,-40,-40,-50,-50,-40,-40,-30,
     -30,-40,-40,-50,-50,-40,-40,-30,
     -30,-40,-40,-50,-50,-40,-40,-30,
     -20,-30,-30,-40,-40,-30,-30,-20,
     -10,-20,-20,-20,-20,-20,-20,-10,
      20, 20,  0,  0,  0,  0, 20, 20,
      20, 30, 10,  0,  0, 10, 30, 20},
};
constexpr int KING_EG[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50};

// White reads the table with the rank flipped (a1 = index 56); Black reads it directly (mirrored board).
constexpr int tableIndex(Color c, Square s) { return c == WHITE ? (s ^ 56) : s; }
constexpr int PHASE_WEIGHT[6] = {0, 1, 1, 2, 4, 0};   // total = 24 at the start position
}  // namespace

int evaluate(const Position& pos) {
    int score = 0;      // White minus Black
    int kingMg = 0, kingEg = 0;
    int phase = 0;
    for (Color c : {WHITE, BLACK}) {
        int sign = c == WHITE ? 1 : -1;
        for (int t = PAWN; t <= QUEEN; ++t) {
            Bitboard bb = pos.of(c, PieceType(t));
            phase += PHASE_WEIGHT[t] * popcount(bb);
            while (bb) {
                Square s = popLsb(bb);
                score += sign * (VALUE[t] + PST[t][tableIndex(c, s)]);
            }
        }
        Square k = pos.kingSq(c);
        kingMg += sign * PST[KING][tableIndex(c, k)];
        kingEg += sign * KING_EG[tableIndex(c, k)];
    }
    if (phase > 24) phase = 24;
    score += (kingMg * phase + kingEg * (24 - phase)) / 24;
    return pos.stm == WHITE ? score : -score;
}
