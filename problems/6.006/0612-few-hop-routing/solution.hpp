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

// Stage 1
inline std::vector<std::int64_t> converge_routes(int n, std::span<const Edge> edges, int source) {
    PRACTICE_TODO();
}

// Stage 2
inline std::vector<std::int64_t> hop_limited_routes(int n, std::span<const Edge> edges, int source,
                                                    int hops) {
    PRACTICE_TODO();
}

// Stage 3
inline int convergence_hops(int n, std::span<const Edge> edges, int source) { PRACTICE_TODO(); }
