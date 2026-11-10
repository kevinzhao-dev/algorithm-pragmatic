#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

inline constexpr int kNoVertex = -1;

class Graph {
public:
    explicit Graph(int vertices) {
        if (vertices < 0) {
            throw std::invalid_argument("negative vertex count");
        }
        adjacency_.resize(static_cast<std::size_t>(vertices));
        in_degree_.resize(static_cast<std::size_t>(vertices));
    }

    int vertex_count() const { return static_cast<int>(adjacency_.size()); }
    std::size_t edge_count() const { return edges_; }

    void add_directed(int from, int to) {
        check(from);
        check(to);
        adjacency_[static_cast<std::size_t>(from)].push_back(to);
        ++in_degree_[static_cast<std::size_t>(to)];
        ++edges_;
    }

    void add_undirected(int a, int b) {
        check(a);
        check(b);
        adjacency_[static_cast<std::size_t>(a)].push_back(b);
        ++in_degree_[static_cast<std::size_t>(b)];
        if (a != b) {
            adjacency_[static_cast<std::size_t>(b)].push_back(a);
            ++in_degree_[static_cast<std::size_t>(a)];
        }
        ++edges_;
    }

    std::span<const int> neighbors(int u) const {
        check(u);
        return adjacency_[static_cast<std::size_t>(u)];
    }

    int out_degree(int u) const {
        check(u);
        return static_cast<int>(adjacency_[static_cast<std::size_t>(u)].size());
    }

    int in_degree(int v) const {
        check(v);
        return in_degree_[static_cast<std::size_t>(v)];
    }

    void check(int vertex) const {
        if (vertex < 0 || vertex >= vertex_count()) {
            throw std::out_of_range("no such vertex");
        }
    }

private:
    std::vector<std::vector<int>> adjacency_;
    std::vector<int> in_degree_;
    std::size_t edges_ = 0;
};

struct BfsTree {
    std::vector<int> dist;
    std::vector<int> parent;
};

inline BfsTree bfs(const Graph& graph, int source) {
    graph.check(source);
    const auto n = static_cast<std::size_t>(graph.vertex_count());
    BfsTree tree{std::vector<int>(n, -1), std::vector<int>(n, kNoVertex)};
    std::vector<int> queue;
    queue.reserve(n);
    tree.dist[static_cast<std::size_t>(source)] = 0;
    queue.push_back(source);
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const int u = queue[head];
        for (const int v : graph.neighbors(u)) {
            if (tree.dist[static_cast<std::size_t>(v)] == -1) {
                tree.dist[static_cast<std::size_t>(v)] = tree.dist[static_cast<std::size_t>(u)] + 1;
                tree.parent[static_cast<std::size_t>(v)] = u;
                queue.push_back(v);
            }
        }
    }
    return tree;
}

inline std::optional<std::vector<int>> shortest_path(const Graph& graph, int source, int target) {
    graph.check(target);
    const BfsTree tree = bfs(graph, source);
    if (tree.dist[static_cast<std::size_t>(target)] == -1) {
        return std::nullopt;
    }
    std::vector<int> path;
    for (int v = target; v != kNoVertex; v = tree.parent[static_cast<std::size_t>(v)]) {
        path.push_back(v);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

inline std::vector<std::vector<int>> level_sets(const Graph& graph, int source) {
    const BfsTree tree = bfs(graph, source);
    const int deepest = *std::max_element(tree.dist.begin(), tree.dist.end());
    std::vector<std::vector<int>> levels(static_cast<std::size_t>(deepest) + 1);
    for (int v = 0; v < graph.vertex_count(); ++v) {
        const int d = tree.dist[static_cast<std::size_t>(v)];
        if (d != -1) {
            levels[static_cast<std::size_t>(d)].push_back(v);
        }
    }
    return levels;
}

// BFS over (vertex, parity of edges walked so far), encoded as 2 * vertex + parity.
inline std::optional<std::vector<int>> shortest_odd_walk(const Graph& graph, int source,
                                                         int target) {
    graph.check(source);
    graph.check(target);
    const auto states = 2 * static_cast<std::size_t>(graph.vertex_count());
    std::vector<int> parent(states, kNoVertex);
    std::vector<char> seen(states, 0);
    std::vector<int> queue;
    const int start = 2 * source;
    const int goal = 2 * target + 1;
    seen[static_cast<std::size_t>(start)] = 1;
    queue.push_back(start);
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const int state = queue[head];
        const int vertex = state / 2;
        const int parity = state % 2;
        for (const int next : graph.neighbors(vertex)) {
            const int successor = 2 * next + (1 - parity);
            if (seen[static_cast<std::size_t>(successor)] == 0) {
                seen[static_cast<std::size_t>(successor)] = 1;
                parent[static_cast<std::size_t>(successor)] = state;
                queue.push_back(successor);
            }
        }
    }
    if (seen[static_cast<std::size_t>(goal)] == 0) {
        return std::nullopt;
    }
    std::vector<int> walk;
    for (int state = goal; state != kNoVertex; state = parent[static_cast<std::size_t>(state)]) {
        walk.push_back(state / 2);
    }
    std::reverse(walk.begin(), walk.end());
    return walk;
}

// Expands whole levels from whichever side has the smaller frontier. Because a level is finished
// before the other side moves, the first collision is already a shortest connection.
inline std::optional<int> separation(const Graph& graph, int a, int b) {
    graph.check(a);
    graph.check(b);
    if (a == b) {
        return 0;
    }
    std::unordered_map<int, int> depth_a{{a, 0}};
    std::unordered_map<int, int> depth_b{{b, 0}};
    std::vector<int> frontier_a{a};
    std::vector<int> frontier_b{b};
    int reached_a = 0;
    int reached_b = 0;
    while (!frontier_a.empty() && !frontier_b.empty()) {
        const bool expand_a = frontier_a.size() <= frontier_b.size();
        auto& frontier = expand_a ? frontier_a : frontier_b;
        auto& own = expand_a ? depth_a : depth_b;
        const auto& other = expand_a ? depth_b : depth_a;
        int& reached = expand_a ? reached_a : reached_b;
        ++reached;
        std::vector<int> next;
        for (const int u : frontier) {
            for (const int v : graph.neighbors(u)) {
                if (other.contains(v)) {
                    return reached + other.at(v);
                }
                if (own.emplace(v, reached).second) {
                    next.push_back(v);
                }
            }
        }
        frontier = std::move(next);
    }
    return std::nullopt;
}
