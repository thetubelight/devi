#include "perft.h"
#include <algorithm>
#include <iostream>
#include "movegen.h"

std::string moveToString(Move m) {
    std::string s;
    s += char('a' + fileOf(moveFrom(m))); s += char('1' + rankOf(moveFrom(m)));
    s += char('a' + fileOf(moveTo(m)));   s += char('1' + rankOf(moveTo(m)));
    if (isPromo(m)) s += "nbrq"[moveFlags(m) & 3];
    return s;
}

PerftTT::PerftTT(size_t megabytes) {
    size_t n = 1;
    while (n * 2 * sizeof(Entry) <= megabytes * 1024 * 1024) n *= 2;   // largest power of two that fits
    table.assign(n, Entry{0, 0});
    mask = n - 1;
}
void PerftTT::clear() { std::fill(table.begin(), table.end(), Entry{0, 0}); }
bool PerftTT::probe(uint64_t key, int depth, uint64_t& nodes) const {
    uint64_t k = mix(key, depth);
    const Entry& e = table[k & mask];
    if (e.key != k) return false;
    nodes = e.nodes;
    return true;
}
void PerftTT::store(uint64_t key, int depth, uint64_t nodes) {
    uint64_t k = mix(key, depth);
    table[k & mask] = Entry{k, nodes};
}

uint64_t perft(Position& pos, int depth, PerftTT* tt) {
    if (depth == 0) return 1;
    uint64_t nodes = 0;
    if (depth >= 2 && tt && tt->probe(pos.key, depth, nodes)) return nodes;

    MoveList ml;
    generate(pos, ml);
    for (Move m : ml) {
        if (!pos.makeIfLegal(m)) continue;
        nodes += (depth == 1) ? 1 : perft(pos, depth - 1, tt);   // bulk-count the leaf level
        pos.unmake();
    }
    if (depth >= 2 && tt) tt->store(pos.key, depth, nodes);
    return nodes;
}

uint64_t divide(Position& pos, int depth, PerftTT* tt) {
    uint64_t total = 0;
    MoveList ml;
    generate(pos, ml);
    for (Move m : ml) {
        if (!pos.makeIfLegal(m)) continue;
        uint64_t n = depth <= 1 ? 1 : perft(pos, depth - 1, tt);
        pos.unmake();
        std::cout << moveToString(m) << ": " << n << "\n";
        total += n;
    }
    return total;
}
