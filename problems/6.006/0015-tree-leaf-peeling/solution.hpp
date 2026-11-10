#pragma once

#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

struct Edge {
    int u;
    int v;
};

// Stage 1
inline bool is_connected(int n, std::span<const Edge> edges) { PRACTICE_TODO(); }
inline bool is_acyclic(int n, std::span<const Edge> edges) { PRACTICE_TODO(); }
inline bool is_tree(int n, std::span<const Edge> edges) { PRACTICE_TODO(); }

// Stage 2
struct Peel {
    int leaf;
    int neighbor;
};

inline std::optional<std::vector<Peel>> peel_order(int n, std::span<const Edge> edges) {
    PRACTICE_TODO();
}

// Stage 3
inline std::vector<int> find_cycle(int n, std::span<const Edge> edges) { PRACTICE_TODO(); }
