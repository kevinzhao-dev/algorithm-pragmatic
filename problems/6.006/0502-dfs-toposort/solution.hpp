#pragma once

#include <span>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include "practice.hpp"

using AdjacencyList = std::vector<std::vector<int>>;

inline constexpr int kNoParent = -1;

// Stage 1
struct DfsForest {
    std::vector<int> parent;
    std::vector<int> finish_order;
};

inline DfsForest dfs(const AdjacencyList& g, int source) { PRACTICE_TODO(); }

// Stage 2
inline DfsForest full_dfs(const AdjacencyList& g) { PRACTICE_TODO(); }
inline std::vector<int> connected_components(const AdjacencyList& g) { PRACTICE_TODO(); }

// Stage 3
struct TopologicalOrder {
    std::vector<int> vertices;
};

struct DirectedCycle {
    std::vector<int> vertices;
};

inline std::variant<TopologicalOrder, DirectedCycle> topological_sort(const AdjacencyList& g) {
    PRACTICE_TODO();
}

// Stage 4
class CycleError : public std::runtime_error {
public:
    explicit CycleError(std::vector<int> cycle) : std::runtime_error("cycle"), cycle_(std::move(cycle)) {}
    const std::vector<int>& cycle() const noexcept { return cycle_; }

private:
    std::vector<int> cycle_;
};

class BuildGraph {
public:
    BuildGraph(int targets, std::span<const std::pair<int, int>> edges) { PRACTICE_TODO(); }

    std::vector<int> rebuild_order(std::span<const int> changed) { PRACTICE_TODO(); }
};
