#include "bitboard.h"

std::string bitboardToString(Bitboard b) {
    std::string s;
    for (int r = 7; r >= 0; --r) {
        s += char('1' + r); s += "  ";
        for (int f = 0; f < 8; ++f) { s += testBit(b, r * 8 + f) ? "X " : ". "; }
        s += '\n';
    }
    s += "   a b c d e f g h\n";
    return s;
}
