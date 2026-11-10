// board.hpp — the types of the Furrow game, shared by your engine, the slow reference and tests.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <stdexcept>

using Bitboard = std::uint64_t;

inline constexpr int kFiles = 7;
inline constexpr int kRanks = 7;
inline constexpr int kSquares = kFiles * kRanks;
inline constexpr Bitboard kAllSquares = (Bitboard{1} << kSquares) - 1;

// Square numbering: a1 = 0, b1 = 1, ..., g1 = 6, a2 = 7, ..., g7 = 48.
constexpr int square_at(int file, int rank) { return rank * kFiles + file; }
constexpr int file_of(int square) { return square % kFiles; }
constexpr int rank_of(int square) { return square / kFiles; }

struct Move {
    std::int8_t from = -1;
    std::int8_t to = -1;

    explicit constexpr operator bool() const { return from >= 0; }
    friend constexpr bool operator==(Move, Move) = default;
    friend constexpr auto operator<=>(const Move&, const Move&) = default;
    friend std::ostream& operator<<(std::ostream& os, Move m) {
        const auto name = [&](int square) {
            os << static_cast<char>('a' + file_of(square)) << static_cast<char>('1' + rank_of(square));
        };
        if (!m) {
            return os << "--";
        }
        name(m.from);
        os << '-';
        name(m.to);
        return os;
    }
};

inline constexpr Move kNoMove{};

// A fixed-capacity move buffer: three plows, at most 24 destinations each.
class MoveList {
public:
    static constexpr int kCapacity = 72;

    void push(Move move) {
        if (count_ == kCapacity) {
            throw std::length_error("MoveList overflow");
        }
        moves_[static_cast<std::size_t>(count_++)] = move;
    }
    void clear() { count_ = 0; }
    int size() const { return count_; }
    bool empty() const { return count_ == 0; }

    Move& operator[](int index) { return moves_[static_cast<std::size_t>(index)]; }
    const Move& operator[](int index) const { return moves_[static_cast<std::size_t>(index)]; }
    Move* begin() { return moves_.data(); }
    Move* end() { return moves_.data() + count_; }
    const Move* begin() const { return moves_.data(); }
    const Move* end() const { return moves_.data() + count_; }

private:
    std::array<Move, kCapacity> moves_{};
    int count_ = 0;
};

struct Position {
    std::array<Bitboard, 2> plows{};
    std::array<Bitboard, 2> furrows{};
    int side = 0;           // 0 = South moves first, 1 = North
    std::uint64_t key = 0;  // Zobrist hash, maintained from Stage 3 on

    // Compares the board and the side to move, not the key.
    friend bool same_board(const Position& a, const Position& b) {
        return a.plows == b.plows && a.furrows == b.furrows && a.side == b.side;
    }
};

constexpr Bitboard bit(int square) { return Bitboard{1} << square; }

// Three plows per side on opposite edges; no furrows yet.
inline Position start_position() {
    Position pos;
    pos.plows[0] = bit(square_at(1, 0)) | bit(square_at(3, 0)) | bit(square_at(5, 0));
    pos.plows[1] = bit(square_at(1, 6)) | bit(square_at(3, 6)) | bit(square_at(5, 6));
    return pos;
}

// Search scores are from the side to move's point of view, in this range.
inline constexpr int kScoreInfinity = 30000;
inline constexpr int kWinScore = 10000;
inline constexpr int kFurrowWeight = 16;
