#include "position.h"
#include <cctype>
#include <cstring>
#include <sstream>

void initEngine() {
    Attacks::init();
    Zobrist::init();
}

namespace {
// Castling rights surviving a move that touches this square (from or to).
int CASTLE_MASK[64];
struct MaskInit {
    MaskInit() {
        for (int& m : CASTLE_MASK) m = 15;
        CASTLE_MASK[A1] = 15 & ~WQ; CASTLE_MASK[E1] = 15 & ~(WK | WQ); CASTLE_MASK[H1] = 15 & ~WK;
        CASTLE_MASK[A8] = 15 & ~BQ; CASTLE_MASK[E8] = 15 & ~(BK | BQ); CASTLE_MASK[H8] = 15 & ~BK;
    }
} maskInit;
const char* PIECE_CHARS = "PNBRQKpnbrqk";
}  // namespace

void Position::clear() {
    std::memset(pieces, 0, sizeof(pieces));
    occ[0] = occ[1] = occAll = 0;
    for (Piece& p : board) p = NO_PIECE;
    stm = WHITE; castling = 0; ep = NO_SQ; halfmove = 0; fullmove = 1; key = 0; histSize = 0;
}

// --- low-level piece helpers: the ONLY functions that touch bitboards, mailbox and key together ---
void Position::putPiece(Piece p, Square s) {
    Bitboard b = bit(s);
    pieces[p] |= b; occ[colorOf(p)] |= b; occAll |= b;
    board[s] = p;
    key ^= Zobrist::piece[p][s];
}
void Position::removePiece(Square s) {
    Piece p = board[s];
    Bitboard b = bit(s);
    pieces[p] ^= b; occ[colorOf(p)] ^= b; occAll ^= b;
    board[s] = NO_PIECE;
    key ^= Zobrist::piece[p][s];
}
void Position::movePiece(Square from, Square to) {   // `to` must be empty
    Piece p = board[from];
    Bitboard ft = bit(from) | bit(to);                // one XOR clears `from` and sets `to`
    pieces[p] ^= ft; occ[colorOf(p)] ^= ft; occAll ^= ft;
    board[from] = NO_PIECE; board[to] = p;
    key ^= Zobrist::piece[p][from] ^ Zobrist::piece[p][to];
}

uint64_t Position::computeKey() const {
    uint64_t k = 0;
    for (Square s = 0; s < 64; ++s) if (board[s] != NO_PIECE) k ^= Zobrist::piece[board[s]][s];
    k ^= Zobrist::castling[castling];
    if (ep != NO_SQ) k ^= Zobrist::ep[fileOf(ep)];
    if (stm == BLACK) k ^= Zobrist::side;
    return k;
}

bool Position::attacked(Square s, Color by) const {
    // A pawn of color `by` attacks s iff a pawn of the opposite color standing on s would attack that pawn's square.
    if (Attacks::pawn[~by][s] & of(by, PAWN)) return true;
    if (Attacks::knight[s] & of(by, KNIGHT)) return true;
    if (Attacks::king[s] & of(by, KING)) return true;
    if (Attacks::bishop(s, occAll) & (of(by, BISHOP) | of(by, QUEEN))) return true;
    if (Attacks::rook(s, occAll) & (of(by, ROOK) | of(by, QUEEN))) return true;
    return false;
}

void Position::make(Move m) {
    Square from = moveFrom(m), to = moveTo(m);
    int flags = moveFlags(m);
    Piece pc = board[from];
    Color us = stm;

    Undo& u = hist[histSize++];
    u.move = m; u.castling = (uint8_t)castling; u.ep = ep; u.halfmove = halfmove; u.key = key; u.captured = NO_PIECE;

    if (ep != NO_SQ) { key ^= Zobrist::ep[fileOf(ep)]; ep = NO_SQ; }
    ++halfmove;

    if (isCapture(m)) {
        Square capSq = (flags == EN_PASSANT) ? (us == WHITE ? to - 8 : to + 8) : to;
        u.captured = board[capSq];
        removePiece(capSq);
        halfmove = 0;
    }
    movePiece(from, to);

    if (typeOf(pc) == PAWN) {
        halfmove = 0;
        if (flags == DOUBLE_PUSH) { ep = (us == WHITE) ? from + 8 : from - 8; key ^= Zobrist::ep[fileOf(ep)]; }
        if (isPromo(m)) { removePiece(to); putPiece(makePiece(us, promoType(m)), to); }
    } else if (flags == KING_CASTLE) {
        movePiece(to + 1, to - 1);     // rook h -> f
    } else if (flags == QUEEN_CASTLE) {
        movePiece(to - 2, to + 1);     // rook a -> d
    }

    key ^= Zobrist::castling[castling];
    castling &= CASTLE_MASK[from] & CASTLE_MASK[to];   // also clears rights when a rook is captured on its home square
    key ^= Zobrist::castling[castling];

    stm = ~us;
    key ^= Zobrist::side;
    if (us == BLACK) ++fullmove;
}

void Position::unmake() {
    const Undo& u = hist[--histSize];
    Move m = u.move;
    Square from = moveFrom(m), to = moveTo(m);
    int flags = moveFlags(m);
    stm = ~stm;
    Color us = stm;
    if (us == BLACK) --fullmove;

    if (isPromo(m)) { removePiece(to); putPiece(makePiece(us, PAWN), from); }
    else movePiece(to, from);

    if (flags == KING_CASTLE) movePiece(to - 1, to + 1);
    else if (flags == QUEEN_CASTLE) movePiece(to + 1, to - 2);

    if (isCapture(m)) {
        Square capSq = (flags == EN_PASSANT) ? (us == WHITE ? to - 8 : to + 8) : to;
        putPiece(u.captured, capSq);
    }
    castling = u.castling; ep = u.ep; halfmove = u.halfmove; key = u.key;   // key restored wholesale
}

void Position::makeNull() {
    Undo& u = hist[histSize++];
    u.move = NULL_MOVE; u.captured = NO_PIECE; u.castling = (uint8_t)castling; u.ep = ep; u.halfmove = halfmove; u.key = key;
    if (ep != NO_SQ) { key ^= Zobrist::ep[fileOf(ep)]; ep = NO_SQ; }
    stm = ~stm;
    key ^= Zobrist::side;
    ++halfmove;
}

void Position::unmakeNull() {
    const Undo& u = hist[--histSize];
    stm = ~stm;
    castling = u.castling; ep = u.ep; halfmove = u.halfmove; key = u.key;
}

bool Position::makeIfLegal(Move m) {
    make(m);
    if (attacked(kingSq(~stm), stm)) { unmake(); return false; }   // ~stm is the side that just moved
    return true;
}

// updated in v1.1 : to fix draw repetition logic
bool Position::isRepetition(int ply) const {
    int lowest = histSize - halfmove;
    if (lowest < 0) lowest = 0;
    const int rootIdx = histSize - ply;           // hist[] index of the root position
    int seen = 0;
    for (int i = histSize - 2; i >= lowest; i -= 2) {
        if (hist[i].key != key) continue;
        if (i > rootIdx) return true;             // earlier occurrence inside the search tree
        if (++seen == 2) return true;             // root/game history: needs a true threefold
    }
    return false;
}

bool Position::insufficientMaterial() const {
    if (pieces[W_PAWN] | pieces[B_PAWN] | pieces[W_ROOK] | pieces[B_ROOK] | pieces[W_QUEEN] | pieces[B_QUEEN]) return false;
    int minors = popcount(pieces[W_KNIGHT] | pieces[B_KNIGHT] | pieces[W_BISHOP] | pieces[B_BISHOP]);
    return minors <= 1;
}

bool Position::setFen(const std::string& fen) {
    Position p;   // parse into a scratch copy so failure leaves *this untouched
    std::istringstream ss(fen);
    std::string placement, side, cast = "-", epStr = "-";
    if (!(ss >> placement >> side)) return false;
    ss >> cast >> epStr;   // optional fields: missing ones keep their defaults
    int hm = 0, fm = 1;
    if (ss >> hm) { if (!(ss >> fm)) fm = 1; }
    if (hm < 0) hm = 0;
    if (fm < 1) fm = 1;

    int r = 7, f = 0;
    for (char c : placement) {
        if (c == '/') { if (f != 8 || r == 0) return false; --r; f = 0; }
        else if (std::isdigit((unsigned char)c)) {
            int n = c - '0';
            if (n < 1 || n > 8 || f + n > 8) return false;
            f += n;
        } else {
            const char* pos = std::strchr(PIECE_CHARS, c);
            if (!pos || f >= 8) return false;
            p.putPiece(Piece(pos - PIECE_CHARS), r * 8 + f);
            ++f;
        }
    }
    if (r != 0 || f != 8) return false;

    if (popcount(p.pieces[W_KING]) != 1 || popcount(p.pieces[B_KING]) != 1) return false;
    if ((p.pieces[W_PAWN] | p.pieces[B_PAWN]) & (RANK_1 | RANK_8)) return false;

    if (side == "w") p.stm = WHITE; else if (side == "b") p.stm = BLACK; else return false;

    if (cast != "-") {
        for (char c : cast) {
            if (c == 'K') p.castling |= WK; else if (c == 'Q') p.castling |= WQ;
            else if (c == 'k') p.castling |= BK; else if (c == 'q') p.castling |= BQ;
            else return false;
        }
    }
    // Drop castling rights that the piece placement cannot support.
    if (p.board[E1] != W_KING) p.castling &= ~(WK | WQ);
    if (p.board[H1] != W_ROOK) p.castling &= ~WK;
    if (p.board[A1] != W_ROOK) p.castling &= ~WQ;
    if (p.board[E8] != B_KING) p.castling &= ~(BK | BQ);
    if (p.board[H8] != B_ROOK) p.castling &= ~BK;
    if (p.board[A8] != B_ROOK) p.castling &= ~BQ;

    if (epStr != "-") {
        if (epStr.size() != 2 || epStr[0] < 'a' || epStr[0] > 'h' || (epStr[1] != '3' && epStr[1] != '6')) return false;
        Square e = (epStr[1] - '1') * 8 + (epStr[0] - 'a');
        // Keep it only if a pawn really just double-pushed there.
        if (p.stm == WHITE && epStr[1] == '6' && p.board[e] == NO_PIECE && p.board[e - 8] == B_PAWN) p.ep = e;
        if (p.stm == BLACK && epStr[1] == '3' && p.board[e] == NO_PIECE && p.board[e + 8] == W_PAWN) p.ep = e;
    }
    p.halfmove = hm; p.fullmove = fm;

    if (p.attacked(p.kingSq(~p.stm), p.stm)) return false;   // side not to move can't be in check
    p.key = p.computeKey();
    p.histSize = 0;
    *this = p;
    return true;
}

std::string Position::fen() const {
    std::string s;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Piece p = board[r * 8 + f];
            if (p == NO_PIECE) { ++empty; continue; }
            if (empty) { s += char('0' + empty); empty = 0; }
            s += PIECE_CHARS[p];
        }
        if (empty) s += char('0' + empty);
        if (r) s += '/';
    }
    s += stm == WHITE ? " w " : " b ";
    if (!castling) s += '-';
    else { if (castling & WK) s += 'K'; if (castling & WQ) s += 'Q'; if (castling & BK) s += 'k'; if (castling & BQ) s += 'q'; }
    s += ' ';
    if (ep == NO_SQ) s += '-';
    else { s += char('a' + fileOf(ep)); s += char('1' + rankOf(ep)); }
    s += ' ' + std::to_string(halfmove) + ' ' + std::to_string(fullmove);
    return s;
}

std::string Position::toString() const {
    std::string s;
    for (int r = 7; r >= 0; --r) {
        s += char('1' + r); s += "  ";
        for (int f = 0; f < 8; ++f) {
            Piece p = board[r * 8 + f];
            s += p == NO_PIECE ? '.' : PIECE_CHARS[p]; s += ' ';
        }
        s += '\n';
    }
    s += "   a b c d e f g h\n\nFEN: " + fen() + "\nKey: " + std::to_string(key) + "\n";
    return s;
}

bool Position::consistent() const {
    Bitboard pb[12] = {}, o[2] = {};
    for (Square s = 0; s < 64; ++s) {
        Piece p = board[s];
        if (p == NO_PIECE) continue;
        pb[p] |= bit(s); o[colorOf(p)] |= bit(s);
    }
    for (int i = 0; i < 12; ++i) if (pb[i] != pieces[i]) return false;
    return o[0] == occ[0] && o[1] == occ[1] && (o[0] | o[1]) == occAll && !(o[0] & o[1]) && computeKey() == key;
}
