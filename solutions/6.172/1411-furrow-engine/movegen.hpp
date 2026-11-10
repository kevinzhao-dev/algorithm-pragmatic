// movegen.hpp — bitboard move generation, make-move and static evaluation for Furrow.
#pragma once

#include <array>
#include <bit>
#include <cstdint>

#include "board.hpp"

namespace furrow_detail {

// One ray per direction per square: every square beyond `square` in that direction.
using RayTable = std::array<std::array<Bitboard, kSquares>, 8>;

// Increasing-index directions come first: their nearest blocker is the lowest set bit.
inline constexpr std::array<std::array<int, 2>, 8> kSlideDeltas{
    {{1, 0}, {0, 1}, {1, 1}, {-1, 1}, {-1, 0}, {0, -1}, {-1, -1}, {1, -1}}};
inline constexpr int kAscendingDirections = 4;

consteval RayTable make_rays() {
    RayTable rays{};
    for (std::size_t direction = 0; direction < kSlideDeltas.size(); ++direction) {
        for (int square = 0; square < kSquares; ++square) {
            int file = file_of(square) + kSlideDeltas[direction][0];
            int rank = rank_of(square) + kSlideDeltas[direction][1];
            Bitboard ray = 0;
            while (file >= 0 && file < kFiles && rank >= 0 && rank < kRanks) {
                ray |= bit(square_at(file, rank));
                file += kSlideDeltas[direction][0];
                rank += kSlideDeltas[direction][1];
            }
            rays[direction][static_cast<std::size_t>(square)] = ray;
        }
    }
    return rays;
}

inline constexpr RayTable kRays = make_rays();

consteval std::array<Bitboard, kSquares> make_orthogonal_neighbours() {
    std::array<Bitboard, kSquares> neighbours{};
    for (int square = 0; square < kSquares; ++square) {
        const int file = file_of(square);
        const int rank = rank_of(square);
        Bitboard mask = 0;
        if (file > 0) {
            mask |= bit(square - 1);
        }
        if (file + 1 < kFiles) {
            mask |= bit(square + 1);
        }
        if (rank > 0) {
            mask |= bit(square - kFiles);
        }
        if (rank + 1 < kRanks) {
            mask |= bit(square + kFiles);
        }
        neighbours[static_cast<std::size_t>(square)] = mask;
    }
    return neighbours;
}

inline constexpr std::array<Bitboard, kSquares> kOrthogonal = make_orthogonal_neighbours();

// Zobrist keys: one per (kind, square) plus one for the side to move, from a fixed PRNG.
inline constexpr int kPieceKinds = 4;

struct ZobristTable {
    std::array<std::array<std::uint64_t, kSquares>, kPieceKinds> piece{};
    std::uint64_t side = 0;
};

constexpr std::uint64_t mix(std::uint64_t z) {
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

consteval ZobristTable make_zobrist() {
    ZobristTable table{};
    std::uint64_t state = 0xF17F17F17F17F17ULL;
    for (auto& per_square : table.piece) {
        for (auto& key : per_square) {
            state += 0x9E3779B97F4A7C15ULL;
            key = mix(state);
        }
    }
    state += 0x9E3779B97F4A7C15ULL;
    table.side = mix(state);
    return table;
}

inline constexpr ZobristTable kZobrist = make_zobrist();

constexpr int plow_kind(int side) { return side; }
constexpr int furrow_kind(int side) { return 2 + side; }

constexpr std::uint64_t key_for(int kind, int square) {
    return kZobrist.piece[static_cast<std::size_t>(kind)][static_cast<std::size_t>(square)];
}

// Squares this slider can land on. Own furrows are transparent but not landable.
inline Bitboard slide_targets(int square, Bitboard blockers, Bitboard own_furrows) {
    Bitboard targets = 0;
    const auto index = static_cast<std::size_t>(square);
    for (std::size_t direction = 0; direction < kSlideDeltas.size(); ++direction) {
        Bitboard ray = kRays[direction][index];
        const Bitboard hits = ray & blockers;
        if (hits != 0) {
            const int nearest = direction < kAscendingDirections
                                    ? std::countr_zero(hits)
                                    : 63 - std::countl_zero(hits);
            ray ^= kRays[direction][static_cast<std::size_t>(nearest)] | bit(nearest);
        }
        targets |= ray;
    }
    return targets & ~own_furrows;
}

}  // namespace furrow_detail

// Returns the full Zobrist hash of the board and side to move.
inline std::uint64_t zobrist_key(const Position& pos) {
    using namespace furrow_detail;
    std::uint64_t key = pos.side != 0 ? kZobrist.side : 0;
    const auto hash_bits = [&](Bitboard bits, int kind) {
        while (bits != 0) {
            key ^= key_for(kind, std::countr_zero(bits));
            bits &= bits - 1;
        }
    };
    hash_bits(pos.plows[0], plow_kind(0));
    hash_bits(pos.plows[1], plow_kind(1));
    hash_bits(pos.furrows[0], furrow_kind(0));
    hash_bits(pos.furrows[1], furrow_kind(1));
    return key;
}

inline Position with_key(Position pos) {
    pos.key = zobrist_key(pos);
    return pos;
}

// Appends every legal move for `side` (whether or not it is that side's turn).
inline void generate_moves_for(const Position& pos, int side, MoveList& out) {
    using namespace furrow_detail;
    const Bitboard own_furrows = pos.furrows[static_cast<std::size_t>(side)];
    const Bitboard enemy_furrows = pos.furrows[static_cast<std::size_t>(1 - side)];
    const Bitboard blockers = pos.plows[0] | pos.plows[1] | enemy_furrows;
    Bitboard plows = pos.plows[static_cast<std::size_t>(side)];
    while (plows != 0) {
        const int from = std::countr_zero(plows);
        plows &= plows - 1;
        Bitboard targets = slide_targets(from, blockers, own_furrows);
        while (targets != 0) {
            const int to = std::countr_zero(targets);
            targets &= targets - 1;
            out.push({static_cast<std::int8_t>(from), static_cast<std::int8_t>(to)});
        }
    }
}

inline void generate_moves(const Position& pos, MoveList& out) {
    out.clear();
    generate_moves_for(pos, pos.side, out);
}

// Number of moves `side` would have if it were on move.
inline int mobility(const Position& pos, int side) {
    using namespace furrow_detail;
    const Bitboard own_furrows = pos.furrows[static_cast<std::size_t>(side)];
    const Bitboard enemy_furrows = pos.furrows[static_cast<std::size_t>(1 - side)];
    const Bitboard blockers = pos.plows[0] | pos.plows[1] | enemy_furrows;
    Bitboard plows = pos.plows[static_cast<std::size_t>(side)];
    int total = 0;
    while (plows != 0) {
        const int from = std::countr_zero(plows);
        plows &= plows - 1;
        total += std::popcount(slide_targets(from, blockers, own_furrows));
    }
    return total;
}

// Requires a legal move. Keeps the Zobrist key in step with the board.
inline Position apply_move(const Position& pos, Move move) {
    using namespace furrow_detail;
    Position next = pos;
    const auto us = static_cast<std::size_t>(pos.side);
    const auto them = 1 - us;
    const Bitboard from = bit(move.from);
    const Bitboard to = bit(move.to);

    next.plows[us] ^= from | to;
    next.furrows[us] |= from;
    next.key ^= key_for(plow_kind(pos.side), move.from) ^ key_for(plow_kind(pos.side), move.to) ^
                key_for(furrow_kind(pos.side), move.from);

    Bitboard flipped = kOrthogonal[static_cast<std::size_t>(move.to)] & next.furrows[them];
    next.furrows[them] ^= flipped;
    next.furrows[us] |= flipped;
    while (flipped != 0) {
        const int square = std::countr_zero(flipped);
        flipped &= flipped - 1;
        next.key ^= key_for(furrow_kind(pos.side), square) ^ key_for(furrow_kind(1 - pos.side), square);
    }

    next.side = 1 - pos.side;
    next.key ^= kZobrist.side;
    return next;
}

inline std::uint64_t perft(const Position& pos, int depth) {
    if (depth == 0) {
        return 1;
    }
    if (depth == 1) {
        return static_cast<std::uint64_t>(mobility(pos, pos.side));
    }
    MoveList moves;
    generate_moves(pos, moves);
    std::uint64_t nodes = 0;
    for (const Move move : moves) {
        nodes += perft(apply_move(pos, move), depth - 1);
    }
    return nodes;
}

// How many enemy furrows would flip if `move` were played from `pos`.
inline int flips_of(const Position& pos, Move move) {
    using namespace furrow_detail;
    return std::popcount(kOrthogonal[static_cast<std::size_t>(move.to)] &
                         pos.furrows[static_cast<std::size_t>(1 - pos.side)]);
}

inline int evaluate(const Position& pos) {
    const auto us = static_cast<std::size_t>(pos.side);
    const int difference = std::popcount(pos.furrows[us]) - std::popcount(pos.furrows[1 - us]);
    const int my_moves = mobility(pos, pos.side);
    if (my_moves == 0) {
        return difference > 0 ? kWinScore + difference : (difference < 0 ? -kWinScore + difference : 0);
    }
    return kFurrowWeight * difference + my_moves - mobility(pos, 1 - pos.side);
}
