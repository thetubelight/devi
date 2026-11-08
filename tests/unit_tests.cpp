// FEN, make/unmake, Zobrist, special-move and search sanity tests.
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <random>
#include "../src/movegen.h"
#include "../src/perft.h"
#include "../src/search.h"

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::cout << "FAIL: " << msg << "  [" #cond "]\n"; ++failures; } } while (0)

static bool same(const Position& a, const Position& b) {
    for (int i = 0; i < 12; ++i) if (a.pieces[i] != b.pieces[i]) return false;
    for (int s = 0; s < 64; ++s) if (a.board[s] != b.board[s]) return false;
    return a.occ[0] == b.occ[0] && a.occ[1] == b.occ[1] && a.occAll == b.occAll && a.stm == b.stm &&
           a.castling == b.castling && a.ep == b.ep && a.halfmove == b.halfmove && a.fullmove == b.fullmove && a.key == b.key;
}

static bool hasMove(const Position& p, const std::string& uci) {
    MoveList ml; generate(p, ml);
    Position q = p;
    for (Move m : ml) if (moveToString(m) == uci && q.makeIfLegal(m)) return true;
    return false;
}

static int legalCount(Position& p) {
    MoveList ml; generate(p, ml);
    int n = 0;
    for (Move m : ml) if (p.makeIfLegal(m)) { ++n; p.unmake(); }
    return n;
}

static const char* FENS[] = {
    START_FEN,
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
    "4k3/8/8/8/8/8/8/4K3 b - - 37 90",
};

static void testFen() {
    for (const char* f : FENS) {
        Position p;
        CHECK(p.setFen(f), std::string("parse ") + f);
        CHECK(p.fen() == f, std::string("roundtrip ") + f + " -> " + p.fen());
        CHECK(p.key == p.computeKey(), "key after setFen");
    }
    Position p;   // missing halfmove/fullmove fields are tolerated (EPD-style data)
    CHECK(p.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -") && p.fen() == START_FEN, "short FEN");
    CHECK(p.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w") && p.castling == 0, "minimal FEN");
    const char* bad[] = {
        "", "garbage", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w KQkq - 0 1", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1",
        "rnbqkbnr/pppppppp/9/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNX w KQkq - 0 1",
        "8/8/8/8/8/8/8/8 w - - 0 1", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e9 0 1",
        "4k3/8/8/8/8/8/8/4K2P w - - 0 1",                      // pawn on back rank
        "4k3/8/8/8/8/8/3R4/4K3 w - - 0 1",   // valid
    };
    for (int i = 0; i < 9; ++i) CHECK(!p.setFen(bad[i]), std::string("reject bad FEN #") + std::to_string(i));
    CHECK(p.setFen(bad[9]), "accept simple valid FEN");
    // Side not to move in check is illegal.
    CHECK(!p.setFen("4k3/4R3/8/8/8/8/8/4K3 w - - 0 1"), "reject: black king in check on white's turn");
}

static void testRandomPlayouts() {
    std::mt19937 rng(12345);
    for (const char* f : FENS) {
        for (int game = 0; game < 60; ++game) {
            Position p; p.setFen(f);
            Position start = p;
            int plies = 0;
            for (; plies < 80; ++plies) {
                MoveList ml; generate(p, ml);
                // pick a random legal move
                Move legal[256]; int n = 0;
                for (Move m : ml) if (p.makeIfLegal(m)) { legal[n++] = m; p.unmake(); }
                if (!n) break;
                Position before = p;
                Move m = legal[rng() % n];
                p.make(m);
                CHECK(p.consistent(), "consistent after make: " + before.fen() + " " + moveToString(m));
                p.unmake();
                CHECK(same(p, before), "make/unmake symmetry: " + before.fen() + " " + moveToString(m));
                p.make(m);
            }
            for (int i = 0; i < plies; ++i) p.unmake();
            CHECK(same(p, start), "full unwind restores start position");
            if (failures > 10) return;
        }
    }
}

static void testSpecialMoves() {
    Position p;
    // En passant that would expose the king along the rank must be illegal.
    p.setFen("8/8/8/8/k2Pp2Q/8/8/3K4 b - d3 0 1");
    CHECK(!hasMove(p, "e4d3"), "pinned en passant is illegal");
    // Plain en passant is legal and removes the right pawn.
    p.setFen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    CHECK(hasMove(p, "e5f6"), "en passant available");
    p.setFen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    { MoveList ml; generate(p, ml); for (Move m : ml) if (moveToString(m) == "e5f6") { p.make(m); break; }
      CHECK(p.board[37] == NO_PIECE && p.board[45] == W_PAWN, "ep captured pawn removed (f5), capturer on f6"); }
    // Castling through an attacked square is forbidden; the other side is fine.
    p.setFen("r3k2r/8/8/8/8/5r2/8/R3K2R w KQkq - 0 1");
    CHECK(!hasMove(p, "e1g1") && hasMove(p, "e1c1"), "no castling through attacked f1; queenside ok");
    // No castling out of check.
    p.setFen("r3k2r/8/8/8/4r3/8/8/R3K2R w KQkq - 0 1");
    CHECK(!hasMove(p, "e1g1") && !hasMove(p, "e1c1"), "no castling while in check");
    // Capturing a rook on its home square removes that castling right.
    p.setFen("r3k2r/8/8/8/8/8/6b1/R3K2R b KQkq - 0 1");
    { MoveList ml; generate(p, ml); for (Move m : ml) if (moveToString(m) == "g2h1") { p.make(m); break; }
      CHECK(!(p.castling & WK) && (p.castling & WQ) && (p.castling & BK), "rook captured on h1 clears WK only"); }
    // Promotions: push (4) + capture (4).
    p.setFen("1n2k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    { MoveList ml; generate(p, ml); int promos = 0; for (Move m : ml) promos += isPromo(m); CHECK(promos == 8, "8 promotion moves, got " + std::to_string(promos)); }
    // Stalemate / checkmate have no legal moves.
    p.setFen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    CHECK(legalCount(p) == 0 && !p.inCheck(), "stalemate detected");
    p.setFen("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1");
    CHECK(legalCount(p) == 0 && p.inCheck(), "checkmate detected");
    // Magic sliders agree with the slow reference on random occupancies.
    std::mt19937_64 rng(99);
    for (int i = 0; i < 20000; ++i) {
        Bitboard occ = rng() & rng(); Square s = rng() % 64;
        CHECK(Attacks::rook(s, occ) == Attacks::slidingAttacks(s, occ, true), "rook magic");
        CHECK(Attacks::bishop(s, occ) == Attacks::slidingAttacks(s, occ, false), "bishop magic");
        if (failures > 10) return;
    }
}

static void testSearch() {
    std::atomic<bool> stop{false};
    struct T { const char* fen; const char* best; int mateIn; };
    T tests[] = {
        {"6k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1", "a1a8", 1},                                                   // back-rank mate
        {"r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4", "h5f7", 1},                     // scholar's mate
        {"1k6/8/1K6/8/8/8/8/7R w - - 0 1", "h1h8", 1},                                                         // rook mate
    };
    for (auto& t : tests) {
        Position p; p.setFen(t.fen);
        SearchLimits lim; lim.depth = 4;
        SearchResult r = search(p, lim, stop, false);
        CHECK(moveToString(r.best) == t.best, std::string("mate in 1 move for ") + t.fen + ", got " + moveToString(r.best));
        CHECK(r.score == MATE - 1, "mate-in-1 score is MATE-1, got " + std::to_string(r.score));
    }
    // Must not hang a queen / must take a free queen.
    Position p; p.setFen("4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1");   // Rxd5 wins the queen for free
    SearchLimits lim; lim.depth = 4;
    SearchResult r = search(p, lim, stop, false);
    CHECK(moveToString(r.best) == "d1d5", std::string("takes free queen, got ") + moveToString(r.best));
    // Draw detection: K vs K scores 0.
    p.setFen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    r = search(p, lim, stop, false);
    CHECK(r.score == 0, "KvK is a draw");
}

int main() {
    initEngine();
    testFen();           std::cout << "fen tests done\n";
    testSpecialMoves();  std::cout << "special-move tests done\n";
    testRandomPlayouts();std::cout << "random playout tests done\n";
    testSearch();        std::cout << "search tests done\n";
    std::cout << (failures ? "FAILED, failures=" : "ALL UNIT TESTS PASSED, failures=") << failures << "\n";
    return failures != 0;
}
