#include "search.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "eval.h"
#include "movegen.h"
#include "perft.h"   // moveToString
#include "tt.h"
#include <cmath>

namespace {
using Clock = std::chrono::steady_clock;
constexpr int MAX_DEPTH = 64;
constexpr int PIECE_ORDER_VALUE[6] = {1, 3, 3, 5, 9, 0};

int LMR[64][64];   // late-move reduction table: [depth][move number]
struct LmrInit {
    LmrInit() {
        for (int d = 1; d < 64; ++d)
            for (int m = 1; m < 64; ++m) LMR[d][m] = int(0.75 + std::log(d) * std::log(m) / 2.25);
    }
} lmrInit;

struct Searcher {
    Position& pos;
    const SearchLimits& lim;
    std::atomic<bool>& stopFlag;
    bool verbose;

    uint64_t nodes = 0;
    bool stopped = false;
    Clock::time_point start = Clock::now();
    int64_t softMs = 0, hardMs = 0;

    Move killers[MAX_PLY][2] = {};
    int  history[2][64][64] = {};
    Move pv[MAX_PLY][MAX_PLY];
    int  pvLen[MAX_PLY] = {};

    Searcher(Position& p, const SearchLimits& l, std::atomic<bool>& s, bool v) : pos(p), lim(l), stopFlag(s), verbose(v) {}

    int64_t elapsed() const { return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count(); }

    void checkStop() {
        if (stopFlag.load(std::memory_order_relaxed)) stopped = true;
        else if (hardMs > 0 && elapsed() >= hardMs) stopped = true;
        else if (lim.nodes && nodes >= lim.nodes) stopped = true;
    }

    void setupTime() {
        if (lim.movetime > 0) {
            hardMs = std::max<int64_t>(1, lim.movetime - lim.moveOverhead);
            softMs = 0;   // fixed time per move: use it all
            return;
        }
        int64_t time = pos.stm == WHITE ? lim.wtime : lim.btime;
        int64_t inc = pos.stm == WHITE ? lim.winc : lim.binc;
        if (time < 0 || lim.infinite) return;   // no clock => depth/nodes/infinite search
        int64_t avail = std::max<int64_t>(1, time - lim.moveOverhead);
        int mtg = lim.movestogo > 0 ? std::min(lim.movestogo, 50) : 30;
        softMs = std::max<int64_t>(1, std::min(avail / mtg + inc / 2, avail / 2));
        hardMs = std::max<int64_t>(1, std::min(softMs * 3, avail * 3 / 4));
    }

    // ---------------- move ordering ----------------
    int scoreMove(Move m, Move pvMove, int ply) const {
        if (m == pvMove) return 1000000;
        if (isCapture(m)) {
            PieceType victim = moveFlags(m) == EN_PASSANT ? PAWN : typeOf(pos.board[moveTo(m)]);
            PieceType attacker = typeOf(pos.board[moveFrom(m)]);
            // MVV-LVA: most valuable victim first, least valuable attacker first.
            return 100000 + PIECE_ORDER_VALUE[victim] * 100 - PIECE_ORDER_VALUE[attacker] * 10 + (isPromo(m) ? 50 : 0);
        }
        if (isPromo(m)) return 90000 + promoType(m);
        if (ply >= 0) {
            if (m == killers[ply][0]) return 80000;
            if (m == killers[ply][1]) return 79999;
        }
        return history[pos.stm][moveFrom(m)][moveTo(m)];
    }

    // Selection-sort style: swap the best remaining move into position `i`.
    static void pickBest(MoveList& ml, int* sc, int i) {
        int best = i;
        for (int j = i + 1; j < ml.count; ++j) if (sc[j] > sc[best]) best = j;
        std::swap(ml.moves[i], ml.moves[best]);
        std::swap(sc[i], sc[best]);
    }

    // ---------------- quiescence ----------------
    int qsearch(int alpha, int beta, int ply) {
        if ((++nodes & 2047) == 0) checkStop();
        if (stopped) return 0;
        if (ply >= MAX_PLY - 1) return evaluate(pos);

        bool inCheck = pos.inCheck();
        int best = -INF;
        if (!inCheck) {   // "stand pat": the side to move may decline all captures
            best = evaluate(pos);
            if (best >= beta) return best;
            if (best > alpha) alpha = best;
        }
        MoveList ml;
        if (inCheck) generate(pos, ml); else generateCaptures(pos, ml);   // in check: must consider all evasions
        int sc[256];
        for (int i = 0; i < ml.count; ++i) sc[i] = scoreMove(ml.moves[i], NULL_MOVE, -1);

        int legal = 0;
        for (int i = 0; i < ml.count; ++i) {
            pickBest(ml, sc, i);
            Move m = ml.moves[i];
            if (!pos.makeIfLegal(m)) continue;
            ++legal;
            int score = -qsearch(-beta, -alpha, ply + 1);
            pos.unmake();
            if (stopped) return 0;
            if (score > best) {
                best = score;
                if (score > alpha) { alpha = score; if (alpha >= beta) break; }
            }
        }
        if (inCheck && legal == 0) return -MATE + ply;
        return best;
    }

    // ---------------- main search ----------------
    bool isDraw(int ply) const { return pos.halfmove >= 100 || pos.isRepetition(ply) || pos.insufficientMaterial(); }

    static int scoreToTT(int s, int ply) { return s >= MATE - MAX_PLY ? s + ply : s <= -MATE + MAX_PLY ? s - ply : s; }
    static int scoreFromTT(int s, int ply) { return s >= MATE - MAX_PLY ? s - ply : s <= -MATE + MAX_PLY ? s + ply : s; }

    int negamax(int depth, int alpha, int beta, int ply, bool allowNull) {
        pvLen[ply] = ply;
        if (ply > 0 && isDraw(ply)) return 0;
        if (ply >= MAX_PLY - 1) return evaluate(pos);

        bool inCheck = pos.inCheck();
        if (inCheck) ++depth;   // check extension
        if (depth <= 0) return qsearch(alpha, beta, ply);

        if ((++nodes & 2047) == 0) checkStop();
        if (stopped) return 0;

        const bool pvNode = beta - alpha > 1;
        const int alphaOrig = alpha;

        // ---- transposition table: reuse earlier results, and its move for ordering ----
        Move ttMove = NULL_MOVE;
        if (const TTEntry* e = TT.probe(pos.key)) {
            ttMove = e->move;
            if (ply > 0 && !pvNode && e->depth >= depth) {   // never cut inside the PV so the PV stays intact
                int s = scoreFromTT(e->score, ply);
                if (e->bound == BOUND_EXACT || (e->bound == BOUND_LOWER && s >= beta) || (e->bound == BOUND_UPPER && s <= alpha))
                    return s;
            }
        }

        // ---- null-move pruning: if passing still beats beta, the real position is surely good enough ----
        if (allowNull && !pvNode && !inCheck && depth >= 3 && ply > 0 &&
            (pos.occ[pos.stm] & ~(pos.of(pos.stm, PAWN) | pos.of(pos.stm, KING))) &&   // zugzwang guard: need a non-pawn piece
            evaluate(pos) >= beta) {
            int R = 2 + depth / 4;
            pos.makeNull();
            int s = -negamax(depth - 1 - R, -beta, -beta + 1, ply + 1, false);
            pos.unmakeNull();
            if (stopped) return 0;
            if (s >= beta) return s >= MATE - MAX_PLY ? beta : s;   // don't trust unproven mate scores
        }

        MoveList ml;
        generate(pos, ml);
        Move first = ply == 0 ? prevBest : ttMove;
        int sc[256];
        for (int i = 0; i < ml.count; ++i) sc[i] = scoreMove(ml.moves[i], first, ply);

        int best = -INF, legal = 0;
        Move bestMove = NULL_MOVE;
        for (int i = 0; i < ml.count; ++i) {
            pickBest(ml, sc, i);
            Move m = ml.moves[i];
            if (!pos.makeIfLegal(m)) continue;
            ++legal;
            const bool quiet = !isCapture(m) && !isPromo(m);
            const bool givesCheck = pos.inCheck();
            const int newDepth = depth - 1;

            int score;
            if (legal == 1) {
                score = -negamax(newDepth, -beta, -alpha, ply + 1, true);   // first move: full window
            } else {
                // Later moves: late-move reduction + zero-window probe (PVS); re-search only if it surprises us.
                int r = 0;
                if (depth >= 3 && legal >= 4 && quiet && !inCheck && !givesCheck) {
                    r = LMR[std::min(depth, 63)][std::min(legal, 63)];
                    if (pvNode && r > 0) --r;
                    r = std::max(0, std::min(r, newDepth - 1));
                }
                score = -negamax(newDepth - r, -alpha - 1, -alpha, ply + 1, true);
                if (score > alpha && r > 0) score = -negamax(newDepth, -alpha - 1, -alpha, ply + 1, true);
                if (score > alpha && score < beta) score = -negamax(newDepth, -beta, -alpha, ply + 1, true);
            }
            pos.unmake();
            if (stopped) return 0;

            if (score > best) {
                best = score;
                bestMove = m;
                if (score > alpha) {
                    alpha = score;
                    pv[ply][ply] = m;   // triangular PV: this move + the child's PV
                    for (int j = ply + 1; j < pvLen[ply + 1]; ++j) pv[ply][j] = pv[ply + 1][j];
                    pvLen[ply] = pvLen[ply + 1];
                    if (alpha >= beta) {
                        if (quiet) {
                            if (killers[ply][0] != m) { killers[ply][1] = killers[ply][0]; killers[ply][0] = m; }
                            int& h = history[pos.stm][moveFrom(m)][moveTo(m)];
                            int bonus = std::min(depth * depth, 400);
                            h += bonus - h * bonus / 16384;   // bounded "gravity" update, stays within [0, 16384]
                        }
                        break;
                    }
                }
            }
        }
        if (legal == 0) return inCheck ? -MATE + ply : 0;   // checkmate or stalemate

        Bound bound = best >= beta ? BOUND_LOWER : (alpha > alphaOrig ? BOUND_EXACT : BOUND_UPPER);
        TT.store(pos.key, depth, scoreToTT(best, ply), bound, bound == BOUND_UPPER ? NULL_MOVE : bestMove);
        return best;
    }

    Move prevBest = NULL_MOVE;   // best root move of the previous completed iteration
    Move rootBest(int ply) const { return ply == 0 ? prevBest : NULL_MOVE; }

    static std::string scoreString(int s) {
        if (std::abs(s) >= MATE - MAX_PLY) {
            int plies = MATE - std::abs(s);
            int moves = (plies + 1) / 2;
            return "mate " + std::to_string(s > 0 ? moves : -moves);
        }
        return "cp " + std::to_string(s);
    }

    SearchResult run() {
        setupTime();
        SearchResult res;
        int maxDepth = lim.depth > 0 ? std::min(lim.depth, MAX_DEPTH) : MAX_DEPTH;

        for (int depth = 1; depth <= maxDepth; ++depth) {
            int score = negamax(depth, -INF, INF, 0, false);
            if (stopped) break;   // discard the incomplete iteration; keep the last completed result
            res.best = pv[0][0]; res.score = score; res.depth = depth; res.nodes = nodes;
            prevBest = res.best;
            if (verbose) {
                int64_t ms = std::max<int64_t>(1, elapsed());
                std::cout << "info depth " << depth << " score " << scoreString(score) << " nodes " << nodes
                          << " nps " << nodes * 1000 / ms << " time " << elapsed() << " pv";
                for (int i = 0; i < pvLen[0]; ++i) std::cout << ' ' << moveToString(pv[0][i]);
                std::cout << std::endl;
            }
            if (softMs > 0 && elapsed() * 5 > softMs * 2) break;   // next iteration likely won't finish
            if (std::abs(score) >= MATE - MAX_PLY && MATE - std::abs(score) <= depth) break;   // forced mate found
        }
        res.nodes = nodes;
        if (res.best == NULL_MOVE) {   // stopped before depth 1 completed: fall back to any legal move
            MoveList ml; generate(pos, ml);
            for (Move m : ml) if (pos.makeIfLegal(m)) { pos.unmake(); res.best = m; break; }
        }
        return res;
    }
};
}  // namespace

SearchResult search(Position& pos, const SearchLimits& limits, std::atomic<bool>& stop, bool verbose) {
    auto searcher = std::make_unique<Searcher>(pos, limits, stop, verbose);
    return searcher->run();
}
