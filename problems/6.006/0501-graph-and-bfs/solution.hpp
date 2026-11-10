#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

inline constexpr int kNoVertex = -1;

// Stage 1
class Graph {
public:
    explicit Graph(int vertices) { PRACTICE_TODO(); }

    int vertex_count() const { PRACTICE_TODO(); }
    std::size_t edge_count() const { PRACTICE_TODO(); }

    void add_directed(int from, int to) { PRACTICE_TODO(); }
    void add_undirected(int a, int b) { PRACTICE_TODO(); }

    std::span<const int> neighbors(int u) const { PRACTICE_TODO(); }
    int out_degree(int u) const { PRACTICE_TODO(); }
    int in_degree(int v) const { PRACTICE_TODO(); }
};

// Stage 2
struct BfsTree {
    std::vector<int> dist;
    std::vector<int> parent;
};

inline BfsTree bfs(const Graph& graph, int source) { PRACTICE_TODO(); }

inline std::optional<std::vector<int>> shortest_path(const Graph& graph, int source, int target) {
    PRACTICE_TODO();
}

inline std::vector<std::vector<int>> level_sets(const Graph& graph, int source) { PRACTICE_TODO(); }

// Stage 3
inline std::optional<std::vector<int>> shortest_odd_walk(const Graph& graph, int source,
                                                         int target) {
    PRACTICE_TODO();
}

// Stage 4
inline std::optional<int> separation(const Graph& graph, int a, int b) { PRACTICE_TODO(); }
