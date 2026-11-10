#pragma once

#include <cstdint>
#include <limits>
#include <span>
#include <vector>

#include "practice.hpp"

struct Arc {
    int to;
    std::int64_t weight;
};

using WeightedGraph = std::vector<std::vector<Arc>>;

inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr int kNoParent = -1;

// Stage 1
inline std::vector<std::int64_t> dag_shortest_paths(const WeightedGraph& g, int source) {
    PRACTICE_TODO();
}

// Stage 2
inline std::vector<int> shortest_path_tree(const WeightedGraph& g, int source,
                                           std::span<const std::int64_t> dist) {
    PRACTICE_TODO();
}

// Stage 3
struct Job {
    std::int64_t duration;
    std::vector<int> depends_on;
};

struct PipelinePlan {
    std::int64_t makespan;
    std::vector<std::int64_t> earliest_start;
    std::vector<std::int64_t> slack;
    std::vector<int> critical_path;
};

inline PipelinePlan plan_pipeline(std::span<const Job> jobs) { PRACTICE_TODO(); }
