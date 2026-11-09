#include "uci.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <thread>
#include "eval.h"
#include "movegen.h"
#include "perft.h"
#include "search.h"
#include "tt.h"

namespace {
const char* ENGINE_NAME = "Devi";
const char* ENGINE_VERSION = "1.1";
const char* AUTHOR = "Rudr";

const char* BENCH_FENS[] = {
    START_FEN,
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
    "2r3k1/pp3ppp/3b4/8/8/2N2P2/PP4PP/2R3K1 w - - 0 1",
    "8/8/4k3/8/8/4K3/4P3/8 w - - 0 1",
    "r2q1rk1/ppp2ppp/2n1bn2/3pp3/1b1PP3/2NB1N2/PPP2PPP/R1BQ1RK1 w - - 0 1",
};

// Convert a UCI move string to an internal move by matching it against the legal moves.
Move parseMove(Position& pos, const std::string& str) {
    MoveList ml;
    generate(pos, ml);
    for (Move m : ml) {
        if (moveToString(m) != str) continue;
        if (pos.makeIfLegal(m)) { pos.unmake(); return m; }
    }
    return NULL_MOVE;
}

struct Engine {
    Position pos;
    std::thread worker;
    std::atomic<bool> stopFlag{false};
    int moveOverhead = 30;

    Engine() { pos.setStartpos(); }
    ~Engine() { stopSearch(); }

    void stopSearch() {
        stopFlag = true;
        if (worker.joinable()) worker.join();
    }

    void cmdPosition(std::istringstream& is) {
        std::string tok, fen;
        is >> tok;
        Position p;
        if (tok == "startpos") { p.setStartpos(); is >> tok; }
        else if (tok == "fen") {
            while (is >> tok && tok != "moves") fen += tok + ' ';
            if (!p.setFen(fen)) { std::cout << "info string invalid FEN, position unchanged" << std::endl; return; }
        } else return;
        if (tok == "moves") {
            while (is >> tok) {
                Move m = parseMove(p, tok);
                if (m == NULL_MOVE) { std::cout << "info string illegal move " << tok << std::endl; break; }
                p.make(m);
            }
        }
        pos = p;
    }

    void cmdGo(std::istringstream& is) {
        stopSearch();
        SearchLimits lim;
        lim.moveOverhead = moveOverhead;
        std::string tok;
        while (is >> tok) {
            if (tok == "depth") is >> lim.depth;
            else if (tok == "movetime") is >> lim.movetime;
            else if (tok == "wtime") is >> lim.wtime;
            else if (tok == "btime") is >> lim.btime;
            else if (tok == "winc") is >> lim.winc;
            else if (tok == "binc") is >> lim.binc;
            else if (tok == "movestogo") is >> lim.movestogo;
            else if (tok == "nodes") is >> lim.nodes;
            else if (tok == "infinite") lim.infinite = true;
            else if (tok == "perft") { int d = 1; is >> d; cmdPerft(d, false); return; }
        }
        stopFlag = false;
        worker = std::thread([this, lim]() {
            Position copy = pos;   // search on a private copy so the UCI thread can keep parsing
            SearchResult r = search(copy, lim, stopFlag, true);
            // UCI: after `go infinite` the engine must not report bestmove until it receives `stop`.
            while (lim.infinite && !stopFlag) std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::cout << "bestmove " << (r.best == NULL_MOVE ? "0000" : moveToString(r.best)) << std::endl;
        });
    }

    void cmdPerft(int depth, bool div) {
        PerftTT tt(128);
        Position p = pos;
        auto t0 = std::chrono::steady_clock::now();
        uint64_t n = div ? divide(p, depth, &tt) : perft(p, depth, &tt);
        double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::cout << "\nNodes: " << n << "\nTime: " << int(s * 1000) << " ms\nNPS: " << (s > 0 ? uint64_t(n / s) : 0) << std::endl;
    }
};
}  // namespace

void runBench(int depth) {
    std::atomic<bool> stop{false};
    uint64_t total = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (const char* fen : BENCH_FENS) {
        Position p; p.setFen(fen);
        TT.clear();
        SearchLimits lim; lim.depth = depth;
        total += search(p, lim, stop, false).nodes;
    }
    double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::cout << total << " nodes " << uint64_t(total / std::max(s, 1e-9)) << " nps (" << int(s * 1000) << " ms)" << std::endl;
}

void uciLoop() {
    Engine eng;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();   // tolerate CRLF input (Windows front-ends)
        std::istringstream is(line);
        std::string cmd;
        is >> cmd;
        if (cmd == "uci") {
            std::cout << "id name " << ENGINE_NAME << ' ' << ENGINE_VERSION << "\nid author " << AUTHOR << "\n"
                      << "option name Hash type spin default 64 min 1 max 4096\n"
                      << "option name Move Overhead type spin default 30 min 0 max 5000\n"
                      << "uciok" << std::endl;
        } else if (cmd == "isready") std::cout << "readyok" << std::endl;
        else if (cmd == "ucinewgame") { eng.stopSearch(); eng.pos.setStartpos(); TT.clear(); }
        else if (cmd == "position") eng.cmdPosition(is);
        else if (cmd == "go") eng.cmdGo(is);
        else if (cmd == "stop") eng.stopSearch();
        else if (cmd == "setoption") {
            std::string tok, name, value;
            is >> tok;   // "name"
            while (is >> tok && tok != "value") name += (name.empty() ? "" : " ") + tok;
            while (is >> tok) value += (value.empty() ? "" : " ") + tok;
            if (name == "Hash") { eng.stopSearch(); TT.resize((size_t)std::max(1, std::atoi(value.c_str()))); }
            else if (name == "Move Overhead") eng.moveOverhead = std::max(0, std::atoi(value.c_str()));
        }
        else if (cmd == "d") std::cout << eng.pos.toString() << std::endl;
        else if (cmd == "eval") std::cout << "eval (side to move): " << evaluate(eng.pos) << " cp" << std::endl;
        else if (cmd == "perft") { int d = 1; is >> d; eng.cmdPerft(d, false); }
        else if (cmd == "divide") { int d = 1; is >> d; eng.cmdPerft(d, true); }
        else if (cmd == "bench") { int d = 6; is >> d; runBench(d); }
        else if (cmd == "quit") { eng.stopSearch(); break; }
        // unknown commands are ignored, as the UCI spec requires
    }
}
