#pragma once

#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

struct Arc {
    int to;
    std::int64_t weight;
};

using WeightedGraph = std::vector<std::vector<Arc>>;
using RelaxedEdge = std::pair<int, int>;

inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();

struct RelaxationTrace {
    std::vector<RelaxedEdge> order;
    std::vector<std::int64_t> dist;
};

// Stage 1
inline RelaxationTrace dag_relaxation_trace(const WeightedGraph& g, int source) { PRACTICE_TODO(); }

// Stage 2
inline RelaxationTrace dijkstra_trace(const WeightedGraph& g, int source) { PRACTICE_TODO(); }

// Stage 3
inline bool is_legal_dijkstra_order(const WeightedGraph& g, int source,
                                    std::span<const RelaxedEdge> order) {
    PRACTICE_TODO();
}
