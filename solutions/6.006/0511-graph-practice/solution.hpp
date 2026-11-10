#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

using AdjacencyMatrix = std::vector<std::vector<int>>;
using AdjacencyList = std::vector<std::vector<int>>;

inline constexpr int kNoParent = -1;

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

inline AdjacencyList sorted_adjacency(const AdjacencyList& g) {
    AdjacencyList copy = g;
    for (auto& adjacency : copy) {
        std::sort(adjacency.begin(), adjacency.end());
    }
    return copy;
}

inline void check_source(const AdjacencyList& g, int source) {
    if (source < 0 || idx(source) >= g.size()) {
        throw std::out_of_range("no such vertex");
    }
}

}  // namespace detail

inline AdjacencyList to_adjacency_list(const AdjacencyMatrix& matrix) {
    AdjacencyList list(matrix.size());
    for (std::size_t u = 0; u < matrix.size(); ++u) {
        if (matrix[u].size() != matrix.size()) {
            throw std::invalid_argument("matrix is not square");
        }
        for (std::size_t v = 0; v < matrix.size(); ++v) {
            if (matrix[u][v] != 0 && matrix[u][v] != 1) {
                throw std::invalid_argument("matrix entries must be 0 or 1");
            }
            if (matrix[u][v] == 1) {
                list[u].push_back(static_cast<int>(v));
            }
        }
    }
    return list;
}

inline AdjacencyMatrix to_adjacency_matrix(const AdjacencyList& list) {
    AdjacencyMatrix matrix(list.size(), std::vector<int>(list.size(), 0));
    for (std::size_t u = 0; u < list.size(); ++u) {
        for (const int v : list[u]) {
            if (v < 0 || detail::idx(v) >= list.size() || matrix[u][detail::idx(v)] == 1) {
                throw std::invalid_argument("neighbor is out of range or repeated");
            }
            matrix[u][detail::idx(v)] = 1;
        }
    }
    return matrix;
}

struct SearchTrace {
    std::vector<int> visit_order;
    std::vector<int> parent;
};

inline SearchTrace bfs_trace(const AdjacencyList& g, int source) {
    detail::check_source(g, source);
    const AdjacencyList adjacency = detail::sorted_adjacency(g);
    SearchTrace trace{{source}, std::vector<int>(g.size(), kNoParent)};
    std::vector<char> seen(g.size(), 0);
    seen[detail::idx(source)] = 1;
    for (std::size_t head = 0; head < trace.visit_order.size(); ++head) {
        const int u = trace.visit_order[head];
        for (const int v : adjacency[detail::idx(u)]) {
            if (seen[detail::idx(v)] == 0) {
                seen[detail::idx(v)] = 1;
                trace.parent[detail::idx(v)] = u;
                trace.visit_order.push_back(v);
            }
        }
    }
    return trace;
}

inline SearchTrace dfs_trace(const AdjacencyList& g, int source) {
    detail::check_source(g, source);
    const AdjacencyList adjacency = detail::sorted_adjacency(g);
    SearchTrace trace{{source}, std::vector<int>(g.size(), kNoParent)};
    std::vector<char> seen(g.size(), 0);
    seen[detail::idx(source)] = 1;
    std::vector<std::pair<int, std::size_t>> stack{{source, 0}};
    while (!stack.empty()) {
        const int u = stack.back().first;
        if (stack.back().second == adjacency[detail::idx(u)].size()) {
            stack.pop_back();
            continue;
        }
        const int v = adjacency[detail::idx(u)][stack.back().second++];
        if (seen[detail::idx(v)] == 0) {
            seen[detail::idx(v)] = 1;
            trace.parent[detail::idx(v)] = u;
            trace.visit_order.push_back(v);
            stack.emplace_back(v, 0);
        }
    }
    return trace;
}

struct EdgeFix {
    int from;
    int to;
    std::vector<int> order;
};

namespace detail {

// Kahn's algorithm that pretends `skip` is absent; nullopt means a cycle remains.
inline std::optional<std::vector<int>> topological_order_without(
    const AdjacencyList& g, std::optional<std::pair<int, int>> skip) {
    std::vector<int> indegree(g.size(), 0);
    for (int u = 0; u < static_cast<int>(g.size()); ++u) {
        for (const int v : g[idx(u)]) {
            if (!(skip && skip->first == u && skip->second == v)) {
                ++indegree[idx(v)];
            }
        }
    }
    std::vector<int> order;
    order.reserve(g.size());
    for (int v = 0; v < static_cast<int>(g.size()); ++v) {
        if (indegree[idx(v)] == 0) {
            order.push_back(v);
        }
    }
    for (std::size_t head = 0; head < order.size(); ++head) {
        const int u = order[head];
        for (const int v : g[idx(u)]) {
            if (!(skip && skip->first == u && skip->second == v) && --indegree[idx(v)] == 0) {
                order.push_back(v);
            }
        }
    }
    if (order.size() != g.size()) {
        return std::nullopt;
    }
    return order;
}

// After peeling every vertex Kahn can remove, each survivor still has a surviving predecessor,
// so walking predecessors must close a cycle.
inline std::vector<int> find_any_cycle(const AdjacencyList& g) {
    std::vector<int> indegree(g.size(), 0);
    for (const auto& adjacency : g) {
        for (const int v : adjacency) {
            ++indegree[idx(v)];
        }
    }
    std::vector<int> ready;
    std::vector<char> peeled(g.size(), 0);
    for (int v = 0; v < static_cast<int>(g.size()); ++v) {
        if (indegree[idx(v)] == 0) {
            ready.push_back(v);
        }
    }
    while (!ready.empty()) {
        const int u = ready.back();
        ready.pop_back();
        peeled[idx(u)] = 1;
        for (const int v : g[idx(u)]) {
            if (--indegree[idx(v)] == 0) {
                ready.push_back(v);
            }
        }
    }
    std::vector<int> predecessor(g.size(), -1);
    int start = -1;
    for (int u = 0; u < static_cast<int>(g.size()); ++u) {
        if (peeled[idx(u)] != 0) {
            continue;
        }
        start = u;
        for (const int v : g[idx(u)]) {
            if (peeled[idx(v)] == 0) {
                predecessor[idx(v)] = u;
            }
        }
    }
    if (start == -1) {
        return {};
    }
    std::vector<char> seen(g.size(), 0);
    int x = start;
    while (seen[idx(x)] == 0) {
        seen[idx(x)] = 1;
        x = predecessor[idx(x)];
    }
    std::vector<int> cycle;
    int y = x;
    do {
        cycle.push_back(y);
        y = predecessor[idx(y)];
    } while (y != x);
    std::reverse(cycle.begin(), cycle.end());
    return cycle;
}

}  // namespace detail

// Every cycle must contain the removed edge, so only the edges of one known cycle are candidates.
inline std::vector<EdgeFix> single_edge_fixes(const AdjacencyList& g) {
    const std::vector<int> cycle = detail::find_any_cycle(g);
    std::vector<EdgeFix> fixes;
    for (std::size_t i = 0; i < cycle.size(); ++i) {
        const int from = cycle[i];
        const int to = cycle[(i + 1) % cycle.size()];
        if (auto order = detail::topological_order_without(g, std::pair{from, to})) {
            fixes.push_back({from, to, std::move(*order)});
        }
    }
    std::sort(fixes.begin(), fixes.end(), [](const EdgeFix& a, const EdgeFix& b) {
        return std::tie(a.from, a.to) < std::tie(b.from, b.to);
    });
    return fixes;
}
