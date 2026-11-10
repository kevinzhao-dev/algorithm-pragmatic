#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

// Stage 1
inline std::int64_t line_game_value(std::span<const int> values) { PRACTICE_TODO(); }

// Stage 2
inline std::int64_t ring_game_value(std::span<const int> slices) { PRACTICE_TODO(); }

// Stage 3
struct Arc {
    std::size_t start;
    std::size_t length;
};

class RingGame {
public:
    explicit RingGame(std::span<const int> slices) { PRACTICE_TODO(); }

    std::int64_t value() const { PRACTICE_TODO(); }
    Arc best_claim(Arc remaining) const { PRACTICE_TODO(); }
};
