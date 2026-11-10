#pragma once

#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

// Stage 1
inline std::vector<int> approx_vertex_cover(int n, std::span<const std::pair<int, int>> edges) {
    PRACTICE_TODO();
}

// Stage 2
inline std::vector<int> greedy_set_cover(int universe, const std::vector<std::vector<int>>& sets) {
    PRACTICE_TODO();
}

// Stage 3
struct TwoWaySplit {
    std::vector<int> side_a;
    long long cost = 0;
};
inline TwoWaySplit approx_partition(std::span<const long long> weights, double eps) {
    PRACTICE_TODO();
}

// Stage 4
inline std::vector<int> approx_tsp(const std::vector<std::vector<long long>>& dist) {
    PRACTICE_TODO();
}
