#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "practice.hpp"

using Conflict = std::pair<std::string, std::string>;

// Stage 1
inline bool can_fit_two_windows(std::span<const Conflict> conflicts) { PRACTICE_TODO(); }

// Stage 2
struct Partition {
    std::vector<std::string> first;
    std::vector<std::string> second;
};

struct OddCycle {
    std::vector<std::string> members;
};

inline std::variant<Partition, OddCycle> plan_two_windows(std::span<const Conflict> conflicts) {
    PRACTICE_TODO();
}

// Stage 3
inline std::optional<std::size_t> first_breaking_conflict(std::span<const Conflict> conflicts) {
    PRACTICE_TODO();
}
