#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <span>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

struct Edge {
    int from;
    int to;
    std::int64_t weight;
};

inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

struct ReducedArc {
    int to;
    std::int64_t weight;
};

inline std::vector<std::vector<ReducedArc>> reduced_adjacency(int n, std::span<const Edge> edges,
                                                              const std::vector<std::int64_t>& h) {
    std::vector<std::vector<ReducedArc>> adjacency(idx(n));
    for (const Edge& e : edges) {
        adjacency[idx(e.from)].push_back({e.to, e.weight + h[idx(e.from)] - h[idx(e.to)]});
    }
    return adjacency;
}

}  // namespace detail

// Bellman-Ford from an implicit supernode with a zero-weight edge to every vertex: starting all
// distances at 0 stands in for the first round, so n rounds are enough to expose a negative cycle.
inline std::optional<std::vector<std::int64_t>> johnson_potentials(int n, std::span<const Edge> edges) {
    std::vector<std::int64_t> h(detail::idx(n), 0);
    for (int round = 0; round < n; ++round) {
        bool changed = false;
        for (const Edge& e : edges) {
            if (h[detail::idx(e.from)] + e.weight < h[detail::idx(e.to)]) {
                h[detail::idx(e.to)] = h[detail::idx(e.from)] + e.weight;
                changed = true;
            }
        }
        if (!changed) {
            return h;
        }
    }
    return std::nullopt;
}

inline std::optional<std::vector<std::vector<std::int64_t>>> johnson(int n, std::span<const Edge> edges) {
    const auto potentials = johnson_potentials(n, edges);
    if (!potentials) {
        return std::nullopt;
    }
    const auto& h = *potentials;
    const auto adjacency = detail::reduced_adjacency(n, edges, h);

    std::vector<std::vector<std::int64_t>> result(detail::idx(n), std::vector<std::int64_t>(detail::idx(n), kUnreachable));
    std::vector<std::int64_t> reduced(detail::idx(n));
    using Entry = std::pair<std::int64_t, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> frontier;
    for (int source = 0; source < n; ++source) {
        std::fill(reduced.begin(), reduced.end(), kUnreachable);
        reduced[detail::idx(source)] = 0;
        frontier.emplace(0, source);
        while (!frontier.empty()) {
            const auto [d, u] = frontier.top();
            frontier.pop();
            if (d != reduced[detail::idx(u)]) {
                continue;
            }
            for (const detail::ReducedArc& arc : adjacency[detail::idx(u)]) {
                if (d + arc.weight < reduced[detail::idx(arc.to)]) {
                    reduced[detail::idx(arc.to)] = d + arc.weight;
                    frontier.emplace(d + arc.weight, arc.to);
                }
            }
        }
        for (int v = 0; v < n; ++v) {
            if (reduced[detail::idx(v)] != kUnreachable) {
                result[detail::idx(source)][detail::idx(v)] =
                    reduced[detail::idx(v)] - h[detail::idx(source)] + h[detail::idx(v)];
            }
        }
    }
    return result;
}

class RoutingTable {
public:
    static std::optional<RoutingTable> build(int n, std::span<const Edge> edges) {
        const auto potentials = johnson_potentials(n, edges);
        if (!potentials) {
            return std::nullopt;
        }
        const auto& h = *potentials;
        const auto adjacency = detail::reduced_adjacency(n, edges, h);

        RoutingTable table;
        table.n_ = n;
        table.dist_.assign(detail::idx(n) * detail::idx(n), kUnreachable);
        table.next_.assign(detail::idx(n) * detail::idx(n), -1);

        // Ties between equally heavy paths are broken by hop count. Each router's choice then has
        // strictly fewer hops left, so a hop-by-hop walk cannot circle a zero-weight cycle.
        using Entry = std::tuple<std::int64_t, int, int>;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> frontier;
        std::vector<std::int64_t> reduced(detail::idx(n));
        std::vector<int> hops(detail::idx(n));
        std::vector<int> first(detail::idx(n));
        for (int source = 0; source < n; ++source) {
            std::fill(reduced.begin(), reduced.end(), kUnreachable);
            std::fill(hops.begin(), hops.end(), std::numeric_limits<int>::max());
            std::fill(first.begin(), first.end(), -1);
            reduced[detail::idx(source)] = 0;
            hops[detail::idx(source)] = 0;
            frontier.emplace(0, 0, source);
            while (!frontier.empty()) {
                const auto [d, count, u] = frontier.top();
                frontier.pop();
                if (d != reduced[detail::idx(u)] || count != hops[detail::idx(u)]) {
                    continue;
                }
                for (const detail::ReducedArc& arc : adjacency[detail::idx(u)]) {
                    const std::int64_t candidate = d + arc.weight;
                    if (std::pair(candidate, count + 1) <
                        std::pair(reduced[detail::idx(arc.to)], hops[detail::idx(arc.to)])) {
                        reduced[detail::idx(arc.to)] = candidate;
                        hops[detail::idx(arc.to)] = count + 1;
                        first[detail::idx(arc.to)] = u == source ? arc.to : first[detail::idx(u)];
                        frontier.emplace(candidate, count + 1, arc.to);
                    }
                }
            }
            for (int v = 0; v < n; ++v) {
                if (reduced[detail::idx(v)] != kUnreachable) {
                    table.dist_[table.slot(source, v)] =
                        reduced[detail::idx(v)] - h[detail::idx(source)] + h[detail::idx(v)];
                    table.next_[table.slot(source, v)] = v == source ? -1 : first[detail::idx(v)];
                }
            }
        }
        return table;
    }

    std::int64_t distance(int from, int to) const {
        check(from);
        check(to);
        return dist_[slot(from, to)];
    }

    std::optional<int> next_hop(int from, int to) const {
        check(from);
        check(to);
        const int hop = next_[slot(from, to)];
        if (hop == -1) {
            return std::nullopt;
        }
        return hop;
    }

    std::optional<std::vector<int>> route(int from, int to) const {
        check(from);
        check(to);
        if (dist_[slot(from, to)] == kUnreachable) {
            return std::nullopt;
        }
        std::vector<int> path{from};
        for (int at = from; at != to;) {
            at = next_[slot(at, to)];
            path.push_back(at);
        }
        return path;
    }

private:
    RoutingTable() = default;

    std::size_t slot(int from, int to) const { return detail::idx(from) * detail::idx(n_) + detail::idx(to); }

    void check(int v) const {
        if (v < 0 || v >= n_) {
            throw std::out_of_range("no such vertex");
        }
    }

    int n_ = 0;
    std::vector<std::int64_t> dist_;
    std::vector<int> next_;
};
