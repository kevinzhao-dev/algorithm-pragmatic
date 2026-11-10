#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "practice.hpp"

inline constexpr int kZoneA = -1;
inline constexpr int kZoneB = -2;

using TileMap = std::vector<std::vector<int>>;
using Corner = std::pair<int, int>;  // (r, c)

// Stage 1
inline std::optional<int> shortest_tray_length(const TileMap& tiles) { PRACTICE_TODO(); }

// Stage 2
inline std::optional<std::vector<Corner>> shortest_tray_route(const TileMap& tiles) {
    PRACTICE_TODO();
}
