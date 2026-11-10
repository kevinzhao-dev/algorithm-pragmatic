#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

#include "practice.hpp"

struct Station {
    std::int64_t x;
    std::int64_t y;
};

// Stage 1
inline std::optional<std::pair<int, int>> find_close_pair(std::span<const Station> stations,
                                                          std::int64_t radius_sq) {
    PRACTICE_TODO();
}

// Stage 2
inline std::optional<std::array<int, 3>> find_close_triple(std::span<const Station> stations,
                                                           std::int64_t radius_sq) {
    PRACTICE_TODO();
}
