#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

struct Edge {
    int to;
    int weight;
};

using Graph = std::vector<std::vector<Edge>>;

// Stage 1
inline std::uint64_t count_paths(const Graph& graph, int source, int target) { PRACTICE_TODO(); }

// Stage 2
struct ParityCounts {
    std::uint64_t even;
    std::uint64_t odd;
};

inline ParityCounts count_paths_by_parity(const Graph& graph, int source, int target) {
    PRACTICE_TODO();
}

// Stage 3
inline std::vector<std::uint64_t> count_paths_by_residue(const Graph& graph, int source, int target,
                                                         int modulus) {
    PRACTICE_TODO();
}

// Stage 4
inline std::optional<std::vector<int>> find_path_with_residue(const Graph& graph, int source,
                                                              int target, int modulus,
                                                              int residue) {
    PRACTICE_TODO();
}
