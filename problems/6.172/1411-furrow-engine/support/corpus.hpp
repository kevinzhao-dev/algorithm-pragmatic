// corpus.hpp — reproducible game positions reached by random play from the start position.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "board.hpp"
#include "slow_board.hpp"

namespace corpus {

class SplitMix {
public:
    explicit SplitMix(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next() {
        std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    int range(int low, int high) {
        return low + static_cast<int>(next() % static_cast<std::uint64_t>(high - low + 1));
    }

private:
    std::uint64_t state_;
};

// Plays `plies` random legal moves from the start (stopping early if the game ends).
inline Position random_game(SplitMix& rng, int plies) {
    slow::Board board = slow::from_position(start_position());
    for (int ply = 0; ply < plies; ++ply) {
        const std::vector<Move> moves = slow::legal_moves(board);
        if (moves.empty()) {
            break;
        }
        board = slow::apply(board, moves[static_cast<std::size_t>(rng.range(0, static_cast<int>(moves.size()) - 1))]);
    }
    return slow::to_position(board);
}

// `count` positions after 0, 1, 2, ... plies of random play, cycling through game phases.
inline std::vector<Position> positions(std::uint64_t seed, std::size_t count, int max_plies = 32) {
    SplitMix rng(seed);
    std::vector<Position> out;
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const int plies = max_plies == 0 ? 0 : static_cast<int>(i) % max_plies;
        out.push_back(random_game(rng, plies));
    }
    return out;
}

}  // namespace corpus
