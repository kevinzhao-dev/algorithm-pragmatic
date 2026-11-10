#pragma once

#include <vector>

#include "practice.hpp"

using AdjacencyMatrix = std::vector<std::vector<int>>;
using AdjacencyList = std::vector<std::vector<int>>;

inline constexpr int kNoParent = -1;

// Stage 1
inline AdjacencyList to_adjacency_list(const AdjacencyMatrix& matrix) { PRACTICE_TODO(); }
inline AdjacencyMatrix to_adjacency_matrix(const AdjacencyList& list) { PRACTICE_TODO(); }

// Stage 2
struct SearchTrace {
    std::vector<int> visit_order;
    std::vector<int> parent;
};

inline SearchTrace bfs_trace(const AdjacencyList& g, int source) { PRACTICE_TODO(); }
inline SearchTrace dfs_trace(const AdjacencyList& g, int source) { PRACTICE_TODO(); }

// Stage 3
struct EdgeFix {
    int from;
    int to;
    std::vector<int> order;
};

inline std::vector<EdgeFix> single_edge_fixes(const AdjacencyList& g) { PRACTICE_TODO(); }
