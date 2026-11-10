#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <span>
#include <vector>

inline constexpr long long kInf = std::numeric_limits<long long>::max() / 4;
using Matrix = std::vector<std::vector<long long>>;

inline Matrix min_plus(const Matrix& a, const Matrix& b) {
    const std::size_t n = a.size();
    Matrix c(n, std::vector<long long>(n, kInf));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t k = 0; k < n; ++k) {
            if (a[i][k] == kInf) {
                continue;
            }
            for (std::size_t j = 0; j < n; ++j) {
                if (b[k][j] != kInf) {
                    c[i][j] = std::min(c[i][j], a[i][k] + b[k][j]);
                }
            }
        }
    }
    return c;
}

inline std::optional<Matrix> apsp_repeated_squaring(const Matrix& w) {
    const std::size_t n = w.size();
    if (n == 0) {
        return Matrix{};
    }
    Matrix d = w;
    // Squaring lg n times covers all paths of up to n edges; a negative cycle needs at most n.
    for (std::size_t hops = 1; hops < n; hops *= 2) {
        d = min_plus(d, d);
    }
    Matrix one_more = min_plus(d, w);
    for (std::size_t i = 0; i < n; ++i) {
        if (one_more[i][i] < 0) {
            return std::nullopt;
        }
    }
    return d;
}

class FloydWarshall {
public:
    explicit FloydWarshall(Matrix w) : dist_(std::move(w)) {
        const std::size_t n = dist_.size();
        next_.assign(n, std::vector<int>(n, -1));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                if (i == j || dist_[i][j] != kInf) {
                    next_[i][j] = static_cast<int>(j);
                }
            }
        }
        for (std::size_t k = 0; k < n; ++k) {
            for (std::size_t i = 0; i < n; ++i) {
                if (dist_[i][k] == kInf) {
                    continue;
                }
                for (std::size_t j = 0; j < n; ++j) {
                    if (dist_[k][j] != kInf && dist_[i][k] + dist_[k][j] < dist_[i][j]) {
                        dist_[i][j] = dist_[i][k] + dist_[k][j];
                        next_[i][j] = next_[i][k];
                    }
                }
            }
        }
        for (std::size_t i = 0; i < n; ++i) {
            if (dist_[i][i] < 0) {
                negative_ = true;
            }
        }
    }

    bool has_negative_cycle() const { return negative_; }
    long long dist(int u, int v) const { return dist_[u][v]; }

    std::vector<int> path(int u, int v) const {
        if (negative_ || dist_[u][v] == kInf) {
            return {};
        }
        std::vector<int> out{u};
        while (u != v) {
            u = next_[u][v];
            out.push_back(u);
        }
        return out;
    }

private:
    Matrix dist_;
    std::vector<std::vector<int>> next_;
    bool negative_ = false;
};

struct Edge {
    int from = 0;
    int to = 0;
    long long weight = 0;
};

inline std::optional<std::vector<long long>> bellman_ford_potentials(
    int n, std::span<const Edge> edges) {
    std::vector<long long> h(n, 0);  // a virtual source with zero-weight edges to every vertex
    for (int round = 0; round <= n; ++round) {
        bool changed = false;
        for (const Edge& e : edges) {
            if (h[e.from] + e.weight < h[e.to]) {
                h[e.to] = h[e.from] + e.weight;
                changed = true;
            }
        }
        if (!changed) {
            return h;
        }
    }
    return std::nullopt;
}

inline std::optional<Matrix> johnson(int n, std::span<const Edge> edges) {
    const auto potentials = bellman_ford_potentials(n, edges);
    if (!potentials) {
        return std::nullopt;
    }
    const auto& h = *potentials;
    std::vector<std::vector<std::pair<int, long long>>> adj(n);
    for (const Edge& e : edges) {
        adj[e.from].emplace_back(e.to, e.weight + h[e.from] - h[e.to]);
    }
    Matrix out(n, std::vector<long long>(n, kInf));
    using Entry = std::pair<long long, int>;
    for (int s = 0; s < n; ++s) {
        auto& d = out[s];
        d[s] = 0;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<>> heap;
        heap.emplace(0, s);
        while (!heap.empty()) {
            const auto [dist_u, u] = heap.top();
            heap.pop();
            if (dist_u > d[u]) {
                continue;
            }
            for (const auto& [v, w] : adj[u]) {
                if (dist_u + w < d[v]) {
                    d[v] = dist_u + w;
                    heap.emplace(d[v], v);
                }
            }
        }
        for (int v = 0; v < n; ++v) {
            if (d[v] != kInf) {
                d[v] += h[v] - h[s];
            }
        }
    }
    return out;
}

struct Constraint {
    int later = 0;
    int earlier = 0;
    long long bound = 0;
};

inline std::optional<std::vector<long long>> solve_difference_constraints(
    int n, std::span<const Constraint> constraints) {
    std::vector<Edge> edges;
    edges.reserve(constraints.size());
    for (const Constraint& c : constraints) {
        edges.push_back({c.earlier, c.later, c.bound});
    }
    return bellman_ford_potentials(n, edges);
}
