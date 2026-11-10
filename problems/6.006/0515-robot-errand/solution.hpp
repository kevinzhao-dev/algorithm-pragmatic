#pragma once

#include <optional>
#include <vector>

#include "practice.hpp"

inline constexpr int kNoBadge = -1;

struct Door {
    int a;
    int b;
    bool two_way;
    int badge;
};

struct Site {
    int locations;
    int dock;
    std::vector<Door> doors;
    std::vector<int> item_locations;
    std::vector<int> badge_locations;
};

// Stages 1 and 2
inline std::optional<int> fewest_doors(const Site& site) { PRACTICE_TODO(); }

// Stage 3
inline std::optional<std::vector<int>> errand_route(const Site& site) { PRACTICE_TODO(); }
