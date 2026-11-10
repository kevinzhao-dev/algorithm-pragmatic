#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "practice.hpp"

// Stage 1
template <typename Key>
class IndexedMinHeap {
public:
    explicit IndexedMinHeap(int capacity) { PRACTICE_TODO(); }

    bool empty() const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    bool contains(int id) const { PRACTICE_TODO(); }
    const Key& key(int id) const { PRACTICE_TODO(); }

    void push(int id, Key key) { PRACTICE_TODO(); }
    std::pair<int, Key> pop_min() { PRACTICE_TODO(); }
    bool decrease_key(int id, Key key) { PRACTICE_TODO(); }
    void clear() { PRACTICE_TODO(); }
};

// Stage 2
struct Arc {
    int to;
    std::int64_t weight;
};

using WeightedGraph = std::vector<std::vector<Arc>>;

inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr int kNoParent = -1;

struct ShortestPaths {
    std::vector<std::int64_t> dist;
    std::vector<int> parent;
};

inline ShortestPaths dijkstra(const WeightedGraph& g, int source) { PRACTICE_TODO(); }

// Stage 3
class RouteEngine {
public:
    explicit RouteEngine(WeightedGraph graph) { PRACTICE_TODO(); }

    std::optional<std::int64_t> distance(int from, int to) { PRACTICE_TODO(); }
    std::optional<std::vector<int>> route(int from, int to) { PRACTICE_TODO(); }
};
