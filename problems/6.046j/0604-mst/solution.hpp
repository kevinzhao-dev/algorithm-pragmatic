#pragma once

#include <span>
#include <vector>

#include "practice.hpp"

struct WeightedEdge {
    int u = 0;
    int v = 0;
    long long w = 0;
};

struct SpanningForest {
    long long total = 0;
    std::vector<int> edges;
};

// Stage 1
inline SpanningForest kruskal(int n, std::span<const WeightedEdge> edges) { PRACTICE_TODO(); }

// Stage 2
inline SpanningForest prim(int n, std::span<const WeightedEdge> edges) { PRACTICE_TODO(); }

// Stage 3
inline std::vector<int> cluster(int n, std::span<const WeightedEdge> edges, int k) {
    PRACTICE_TODO();
}
