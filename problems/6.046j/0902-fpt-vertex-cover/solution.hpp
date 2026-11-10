#pragma once

#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

using EdgeList = std::span<const std::pair<int, int>>;

// Stage 1
inline std::optional<std::vector<int>> vc_search(int n, EdgeList edges, int k) { PRACTICE_TODO(); }

// Stage 2
struct Kernel {
    bool no_instance = false;
    int n = 0;
    std::vector<std::pair<int, int>> edges;
    int k = 0;
    std::vector<int> forced;
    std::vector<int> to_original;
};
inline Kernel kernelize(int n, EdgeList edges, int k) { PRACTICE_TODO(); }

// Stage 3
inline std::optional<std::vector<int>> vc_fpt(int n, EdgeList edges, int k) { PRACTICE_TODO(); }

// Stage 4
inline std::vector<int> min_vertex_cover(int n, EdgeList edges) { PRACTICE_TODO(); }
