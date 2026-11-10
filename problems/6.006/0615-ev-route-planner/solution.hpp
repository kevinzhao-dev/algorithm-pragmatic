#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "practice.hpp"

struct Road {
    int a;
    int b;
    std::int64_t minutes;
};

struct RoadMap {
    std::vector<int> elevation;
    std::vector<char> has_charger;
    std::vector<Road> roads;
    int capacity;
    std::int64_t minutes_per_unit;
};

// Stages 1 and 2
inline std::optional<std::int64_t> fastest_time(const RoadMap& map, int source, int target) {
    PRACTICE_TODO();
}

// Stage 3
struct Stop {
    int location;
    int charge_units;
};

struct RoutePlan {
    std::int64_t total_minutes;
    std::vector<Stop> stops;
};

inline std::optional<RoutePlan> fastest_plan(const RoadMap& map, int source, int target) {
    PRACTICE_TODO();
}
