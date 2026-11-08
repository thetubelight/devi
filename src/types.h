// Core types and move encoding.
#pragma once
#include <cstdint>
#include <string>

using Bitboard = uint64_t;
using Square   = int;   // a1 = 0, b1 = 1, ... h1 = 7, a2 = 8, ... h8 = 63
using Move     = uint16_t;

enum Color : int { WHITE = 0, BLACK = 1 };
constexpr Color operator~(Color c) { return Color(c ^ 1); }

enum PieceType : int { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE_TYPE };

// Piece = color * 6 + type, so the order matches the FEN string "PNBRQKpnbrqk".
enum Piece : int {
    W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    NO_PIECE
};
constexpr Piece     makePiece(Color c, PieceType t) { return Piece(c * 6 + t); }
constexpr Color     colorOf(Piece p) { return Color(p / 6); }
constexpr PieceType typeOf(Piece p) { return PieceType(p % 6); }

constexpr Square NO_SQ = -1;
// little endian rank-file mapping : s = rank * 8 + file
constexpr int fileOf(Square s) { return s & 7; } // equivalent to s % 8 => file
constexpr int rankOf(Square s) { return s >> 3; } //similar to s / 8 => rank

enum : Square {
    A1 = 0, B1, C1, D1, E1, F1, G1, H1,
    A8 = 56, B8, C8, D8, E8, F8, G8, H8
};

// Castling rights bitmask.
enum : int { WK = 1, WQ = 2, BK = 4, BQ = 8 };

// Move = from (6 bits) | to (6 bits) << 6 | flags (4 bits) << 12
// Flags follow the Chess Programming Wiki scheme: bit 2 = capture, bit 3 = promotion.
enum MoveFlag : int {
    QUIET = 0, DOUBLE_PUSH = 1, KING_CASTLE = 2, QUEEN_CASTLE = 3,
    CAPTURE = 4, EN_PASSANT = 5,
    PROMO_N = 8, PROMO_B = 9, PROMO_R = 10, PROMO_Q = 11,
    PROMO_CAP_N = 12, PROMO_CAP_B = 13, PROMO_CAP_R = 14, PROMO_CAP_Q = 15
};
//why store different flags(specially quiet promos and promo by captures)seperately ?
//Having distinct flags allows the move-generator and search algorithms to instantly identify and sort a promotion-capture into the highest priority queue without needing to look up the board array or bitboard to see if the target square was occupied


constexpr Move   makeMove(Square f, Square t, int flags = QUIET) { return Move(f | (t << 6) | (flags << 12)); }
constexpr Square moveFrom(Move m) { return m & 63; }
constexpr Square moveTo(Move m) { return (m >> 6) & 63; }
constexpr int    moveFlags(Move m) { return m >> 12; }
constexpr bool   isCapture(Move m) { return moveFlags(m) & 4; }   // includes en passant and promo-captures
constexpr bool   isPromo(Move m) { return moveFlags(m) & 8; }
constexpr PieceType promoType(Move m) { return PieceType((moveFlags(m) & 3) + KNIGHT); }
constexpr Move   NULL_MOVE = 0;
