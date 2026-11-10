#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

struct Edge {
    int to;
    int weight;
};

using Graph = std::vector<std::vector<Edge>>;

namespace detail {

inline std::vector<int> topological_order(const Graph& graph) {
    const auto n = static_cast<int>(graph.size());
    std::vector<int> indegree(graph.size(), 0);
    for (const auto& edges : graph) {
        for (const Edge& edge : edges) {
            ++indegree[static_cast<std::size_t>(edge.to)];
        }
    }
    std::vector<int> order;
    order.reserve(graph.size());
    for (int v = 0; v < n; ++v) {
        if (indegree[static_cast<std::size_t>(v)] == 0) {
            order.push_back(v);
        }
    }
    for (std::size_t head = 0; head < order.size(); ++head) {
        for (const Edge& edge : graph[static_cast<std::size_t>(order[head])]) {
            if (--indegree[static_cast<std::size_t>(edge.to)] == 0) {
                order.push_back(edge.to);
            }
        }
    }
    if (order.size() != graph.size()) {
        throw std::invalid_argument("graph contains a cycle");
    }
    return order;
}

inline void check_vertices(const Graph& graph, int source, int target) {
    const auto n = static_cast<int>(graph.size());
    if (source < 0 || source >= n || target < 0 || target >= n) {
        throw std::out_of_range("vertex out of range");
    }
}

// The residue of `value` modulo `modulus`, normalized to [0, modulus) even for negative values.
inline int normalized_residue(int value, int modulus) {
    return ((value % modulus) + modulus) % modulus;
}

}  // namespace detail

inline std::vector<std::uint64_t> count_paths_by_residue(const Graph& graph, int source, int target,
                                                         int modulus) {
    if (modulus < 1) {
        throw std::invalid_argument("modulus must be positive");
    }
    detail::check_vertices(graph, source, target);
    const std::vector<int> order = detail::topological_order(graph);
    const auto k = static_cast<std::size_t>(modulus);
    std::vector<std::uint64_t> ways(graph.size() * k, 0);
    ways[static_cast<std::size_t>(source) * k] = 1;
    for (const int v : order) {
        const auto from = static_cast<std::size_t>(v) * k;
        for (const Edge& edge : graph[static_cast<std::size_t>(v)]) {
            const auto shift =
                static_cast<std::size_t>(detail::normalized_residue(edge.weight, modulus));
            const auto to = static_cast<std::size_t>(edge.to) * k;
            for (std::size_t r = 0; r < k; ++r) {
                ways[to + (r + shift) % k] += ways[from + r];
            }
        }
    }
    const auto at = static_cast<std::size_t>(target) * k;
    return {ways.begin() + static_cast<std::ptrdiff_t>(at),
            ways.begin() + static_cast<std::ptrdiff_t>(at + k)};
}

inline std::uint64_t count_paths(const Graph& graph, int source, int target) {
    return count_paths_by_residue(graph, source, target, 1)[0];
}

struct ParityCounts {
    std::uint64_t even;
    std::uint64_t odd;
};

inline ParityCounts count_paths_by_parity(const Graph& graph, int source, int target) {
    const auto counts = count_paths_by_residue(graph, source, target, 2);
    return {counts[0], counts[1]};
}

inline std::optional<std::vector<int>> find_path_with_residue(const Graph& graph, int source,
                                                              int target, int modulus,
                                                              int residue) {
    if (modulus < 1) {
        throw std::invalid_argument("modulus must be positive");
    }
    detail::check_vertices(graph, source, target);
    const std::vector<int> order = detail::topological_order(graph);
    const auto k = static_cast<std::size_t>(modulus);
    constexpr int kUnreached = -1;
    constexpr int kStart = -2;
    // parent of state (v, r), stored at v * k + r.
    std::vector<int> parent(graph.size() * k, kUnreached);
    parent[static_cast<std::size_t>(source) * k] = kStart;
    for (const int v : order) {
        for (std::size_t r = 0; r < k; ++r) {
            const std::size_t state = static_cast<std::size_t>(v) * k + r;
            if (parent[state] == kUnreached) {
                continue;
            }
            for (const Edge& edge : graph[static_cast<std::size_t>(v)]) {
                const auto shift =
                    static_cast<std::size_t>(detail::normalized_residue(edge.weight, modulus));
                const std::size_t next =
                    static_cast<std::size_t>(edge.to) * k + (r + shift) % k;
                if (parent[next] == kUnreached) {
                    parent[next] = static_cast<int>(state);
                }
            }
        }
    }

    int state = target * modulus + residue;
    if (parent[static_cast<std::size_t>(state)] == kUnreached) {
        return std::nullopt;
    }
    std::vector<int> path;
    while (state != kStart) {
        path.push_back(state / modulus);
        state = parent[static_cast<std::size_t>(state)];
    }
    std::ranges::reverse(path);
    return path;
}
