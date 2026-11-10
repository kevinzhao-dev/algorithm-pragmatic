#pragma once

#include <stdexcept>
#include <vector>

#include "practice.hpp"

class FlowNetwork {
public:
    struct PathFlow {
        std::vector<int> edges;
        long long amount = 0;
    };

    explicit FlowNetwork(int n) { PRACTICE_TODO(); }

    // Stage 1
    int add_edge(int u, int v, long long capacity) { PRACTICE_TODO(); }
    long long max_flow_dfs(int s, int t) { PRACTICE_TODO(); }
    long long flow_on(int id) const { PRACTICE_TODO(); }

    // Stage 2
    long long max_flow_bfs(int s, int t) { PRACTICE_TODO(); }
    int augmentations() const { PRACTICE_TODO(); }

    // Stage 3
    std::vector<bool> min_cut_side() const { PRACTICE_TODO(); }
    std::vector<int> cut_edges() const { PRACTICE_TODO(); }

    // Stage 4
    std::vector<PathFlow> decompose() const { PRACTICE_TODO(); }
};
