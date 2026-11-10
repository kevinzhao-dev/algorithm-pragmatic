#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <utility>
#include <vector>

struct Edge {
    int u;
    int v;
};

struct Peel {
    int leaf;
    int neighbor;
};

namespace detail {

// Adjacency lists that remember which edge each arc came from, so parallel edges stay distinct.
struct Graph {
    struct Arc {
        int to;
        int edge;
    };

    explicit Graph(int n, std::span<const Edge> edges)
        : adjacency(static_cast<std::size_t>(n)), degree(static_cast<std::size_t>(n), 0) {
        for (std::size_t e = 0; e < edges.size(); ++e) {
            const auto [u, v] = edges[e];
            adjacency[index(u)].push_back({v, static_cast<int>(e)});
            ++degree[index(u)];
            // A self-loop gives its vertex degree 2 with a single entry in the list.
            ++degree[index(v)];
            if (u != v) {
                adjacency[index(v)].push_back({u, static_cast<int>(e)});
            }
        }
    }

    static std::size_t index(int v) { return static_cast<std::size_t>(v); }

    int vertex_count() const { return static_cast<int>(adjacency.size()); }

    std::vector<std::vector<Arc>> adjacency;
    std::vector<int> degree;
};

inline int component_count(const Graph& graph) {
    const int n = graph.vertex_count();
    std::vector<char> seen(static_cast<std::size_t>(n), 0);
    std::vector<int> frontier;
    int components = 0;
    for (int start = 0; start < n; ++start) {
        if (seen[Graph::index(start)]) {
            continue;
        }
        ++components;
        seen[Graph::index(start)] = 1;
        frontier.push_back(start);
        while (!frontier.empty()) {
            const int v = frontier.back();
            frontier.pop_back();
            for (const auto& arc : graph.adjacency[Graph::index(v)]) {
                if (!seen[Graph::index(arc.to)]) {
                    seen[Graph::index(arc.to)] = 1;
                    frontier.push_back(arc.to);
                }
            }
        }
    }
    return components;
}

// Removes degree-1 vertices until none remain and returns the removals. `removed` records which
// vertices are gone; whatever is left has degree 0 or at least 2 in the remaining graph.
inline std::vector<Peel> peel(Graph& graph, std::vector<char>& removed) {
    const int n = graph.vertex_count();
    removed.assign(static_cast<std::size_t>(n), 0);
    std::vector<int> ready;
    for (int v = 0; v < n; ++v) {
        if (graph.degree[Graph::index(v)] == 1) {
            ready.push_back(v);
        }
    }
    std::vector<Peel> removals;
    while (!ready.empty()) {
        const int leaf = ready.back();
        ready.pop_back();
        if (removed[Graph::index(leaf)] || graph.degree[Graph::index(leaf)] != 1) {
            continue;
        }
        for (const auto& arc : graph.adjacency[Graph::index(leaf)]) {
            if (!removed[Graph::index(arc.to)]) {
                removals.push_back({leaf, arc.to});
                removed[Graph::index(leaf)] = 1;
                graph.degree[Graph::index(leaf)] = 0;
                if (--graph.degree[Graph::index(arc.to)] == 1) {
                    ready.push_back(arc.to);
                }
                break;
            }
        }
    }
    return removals;
}

}  // namespace detail

inline bool is_connected(int n, std::span<const Edge> edges) {
    return detail::component_count(detail::Graph(n, edges)) == 1;
}

inline bool is_acyclic(int n, std::span<const Edge> edges) {
    return static_cast<int>(edges.size()) + detail::component_count(detail::Graph(n, edges)) == n;
}

inline bool is_tree(int n, std::span<const Edge> edges) {
    return is_connected(n, edges) && is_acyclic(n, edges);
}

inline std::optional<std::vector<Peel>> peel_order(int n, std::span<const Edge> edges) {
    if (static_cast<int>(edges.size()) != n - 1) {
        return std::nullopt;
    }
    detail::Graph graph(n, edges);
    std::vector<char> removed;
    auto removals = detail::peel(graph, removed);
    if (static_cast<int>(removals.size()) != n - 1) {
        return std::nullopt;
    }
    return removals;
}

inline std::vector<int> find_cycle(int n, std::span<const Edge> edges) {
    detail::Graph graph(n, edges);
    std::vector<char> removed;
    detail::peel(graph, removed);

    int start = 0;
    while (removed[detail::Graph::index(start)]) {
        ++start;
    }
    std::vector<char> used(edges.size(), 0);
    std::vector<int> cycle;
    int v = start;
    do {
        cycle.push_back(v);
        for (const auto& arc : graph.adjacency[detail::Graph::index(v)]) {
            const auto edge = static_cast<std::size_t>(arc.edge);
            if (!used[edge] && !removed[detail::Graph::index(arc.to)]) {
                used[edge] = 1;
                v = arc.to;
                break;
            }
        }
    } while (v != start);
    return cycle;
}
