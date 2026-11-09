// Board state: bitboards + mailbox mirror + Zobrist key, FEN I/O, make/unmake.
#pragma once
#include <string>
#include "attacks.h"
#include "bitboard.h"
#include "types.h"
#include "zobrist.h"

void initEngine();   // initializes attack tables and Zobrist keys; call once at startup

struct Undo {
    Move     move;
    Piece    captured;
    uint8_t  castling;
    int      ep;
    int      halfmove;
    uint64_t key;      // key of the position *before* the move (used for repetition detection)
};

constexpr const char* START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr int MAX_HISTORY = 2048;

class Position {
public:
    Bitboard pieces[12];   // indexed by Piece
    Bitboard occ[2];       // per-color occupancy
    Bitboard occAll;
    Piece    board[64];    // mailbox mirror: O(1) "what is on this square"
    Color    stm;
    int      castling;     // WK | WQ | BK | BQ
    Square   ep;           // en-passant target square (the square *behind* the pushed pawn) or NO_SQ
    int      halfmove, fullmove;
    uint64_t key;
    Undo     hist[MAX_HISTORY];
    int      histSize;

    Position() { clear(); }
    void clear();
    bool setFen(const std::string& fen);   // returns false (leaving *this untouched) on malformed/illegal input
    void setStartpos() { setFen(START_FEN); }
    std::string fen() const;
    std::string toString() const;          // human-readable board for debugging

    void make(Move m);                     // no legality check
    void unmake();
    void makeNull();                       // pass the turn (null-move pruning)
    void unmakeNull();
    bool makeIfLegal(Move m);              // make; if own king is left in check, unmake and return false

    bool   attacked(Square s, Color by) const;
    Square kingSq(Color c) const { return lsb(pieces[makePiece(c, KING)]); }
    bool   inCheck() const { return attacked(kingSq(stm), ~stm); }
    Bitboard of(Color c, PieceType t) const { return pieces[makePiece(c, t)]; }

    // updated in v1.1 : to fix draw repetition logic 
    bool isRepetition(int ply) const;             // current position occurred earlier in the reversible history

    bool insufficientMaterial() const;     // K vs K, K+minor vs K
    uint64_t computeKey() const;           // from scratch (the incremental key must always equal this)
    bool consistent() const;               // debug: recompute everything and compare

private:
    void putPiece(Piece p, Square s);
    void removePiece(Square s);
    void movePiece(Square from, Square to);
};
