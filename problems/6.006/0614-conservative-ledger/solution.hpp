#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

struct Transition {
    int from;
    int to;
    std::int64_t delta;
};

// Stages 1 and 3
inline bool is_conservative(int states, std::span<const Transition> transitions) { PRACTICE_TODO(); }

// Stage 2
inline std::optional<std::vector<std::size_t>> find_unbalanced_cycle(
    int states, std::span<const Transition> transitions) {
    PRACTICE_TODO();
}
