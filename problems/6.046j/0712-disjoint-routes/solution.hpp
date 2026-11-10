#pragma once

#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

using EdgeList = std::span<const std::pair<int, int>>;

// Stage 1
inline std::optional<std::vector<std::vector<int>>> edge_disjoint_routes(int n, EdgeList edges,
                                                                         std::span<const int> sources,
                                                                         int target) {
    PRACTICE_TODO();
}

// Stage 2
struct PartialRouting {
    std::vector<std::vector<int>> routes;
    std::vector<int> cut_edges;
    std::vector<int> free_sources;
};
inline PartialRouting best_effort_routes(int n, EdgeList edges, std::span<const int> sources,
                                         int target) {
    PRACTICE_TODO();
}

// Stage 3
inline std::optional<std::vector<std::vector<int>>> routes_with_hub_limits(
    int n, EdgeList edges, std::span<const int> sources, int target,
    std::span<const int> hub_limit) {
    PRACTICE_TODO();
}
