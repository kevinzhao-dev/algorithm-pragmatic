#pragma once

#include <cstdint>
#include <limits>
#include <span>
#include <vector>

#include "practice.hpp"

struct Edge {
    int from;
    int to;
    std::int64_t weight;
};

inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr std::int64_t kMinusInfinity = std::numeric_limits<std::int64_t>::min();

// Stage 1
inline std::vector<std::int64_t> shortest_within_edges(int vertices, std::span<const Edge> edges,
                                                       int source, int max_edges) {
    PRACTICE_TODO();
}

class AllPairs {
public:
    // Stage 2
    AllPairs(int vertices, std::span<const Edge> edges) { PRACTICE_TODO(); }

    std::int64_t distance(int from, int to) const { PRACTICE_TODO(); }

    // Stage 3
    std::vector<int> path(int from, int to) const { PRACTICE_TODO(); }

    // Stage 4
    bool has_negative_cycle() const { PRACTICE_TODO(); }
};
