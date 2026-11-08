// Perft suite: node counts for the standard test positions (Chess Programming Wiki).
// Usage: perft_tests          -> depth <= 5 (quick)
//        perft_tests --deep   -> every listed depth, including 6 and 7
#include <chrono>
#include <cstring>
#include <iostream>
#include "../src/perft.h"

struct Case { const char* name; const char* fen; std::vector<uint64_t> counts; };   // counts[i] = perft(i+1)

static const std::vector<Case> CASES = {
    {"startpos", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
     {20, 400, 8902, 197281, 4865609, 119060324, 3195901860ULL}},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
     {48, 2039, 97862, 4085603, 193690690ULL, 8031647685ULL}},
    {"position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
     {14, 191, 2812, 43238, 674624, 11030083, 178633661ULL}},
    {"position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
     {6, 264, 9467, 422333, 15833292, 706045033ULL}},
    {"position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
     {44, 1486, 62379, 2103487, 89941194ULL}},
    {"position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
     {46, 2079, 89890, 3894594, 164075551ULL}},
};

int main(int argc, char** argv) {
    bool deep = argc > 1 && !std::strcmp(argv[1], "--deep");
    initEngine();
    PerftTT tt(256);
    int failures = 0;
    for (const Case& c : CASES) {
        Position p;
        if (!p.setFen(c.fen)) { std::cout << "FAIL " << c.name << ": bad FEN\n"; ++failures; continue; }
        for (size_t d = 1; d <= c.counts.size(); ++d) {
            if (!deep && d > 5) break;
            // skip the very deepest levels for the huge ones unless hashing makes them cheap: opt-in via --deep
            if (deep && d == 7 && std::strcmp(c.name, "startpos") && std::strcmp(c.name, "position3")) continue;
            auto t0 = std::chrono::steady_clock::now();
            uint64_t hashed = perft(p, (int)d, &tt);
            double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            bool ok = hashed == c.counts[d - 1];
            // cross-check against unhashed perft while it is cheap, to catch TT key/depth bugs
            if (ok && d <= 4) ok = perft(p, (int)d, nullptr) == c.counts[d - 1];
            std::cout << (ok ? "PASS " : "FAIL ") << c.name << " d" << d << ": " << hashed
                      << " (expected " << c.counts[d - 1] << ") " << s << "s\n";
            if (!ok) ++failures;
        }
        tt.clear();
    }
    std::cout << (failures ? "FAILED: " : "ALL PERFT TESTS PASSED, failures=") << failures << "\n";
    return failures != 0;
}
