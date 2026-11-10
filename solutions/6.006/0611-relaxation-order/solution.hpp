#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <set>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

struct Arc {
    int to;
    std::int64_t weight;
};

using WeightedGraph = std::vector<std::vector<Arc>>;
using RelaxedEdge = std::pair<int, int>;

inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();

struct RelaxationTrace {
    std::vector<RelaxedEdge> order;
    std::vector<std::int64_t> dist;
};

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

inline void check_source(const WeightedGraph& g, int source) {
    if (source < 0 || idx(source) >= g.size()) {
        throw std::out_of_range("no such vertex");
    }
}

inline std::vector<Arc> sorted_arcs(const std::vector<Arc>& arcs) {
    std::vector<Arc> copy = arcs;
    std::sort(copy.begin(), copy.end(), [](const Arc& a, const Arc& b) { return a.to < b.to; });
    return copy;
}

inline std::vector<char> reachable_from(const WeightedGraph& g, int source) {
    std::vector<char> seen(g.size(), 0);
    std::vector<int> stack{source};
    seen[idx(source)] = 1;
    while (!stack.empty()) {
        const int u = stack.back();
        stack.pop_back();
        for (const Arc& arc : g[idx(u)]) {
            if (seen[idx(arc.to)] == 0) {
                seen[idx(arc.to)] = 1;
                stack.push_back(arc.to);
            }
        }
    }
    return seen;
}

}  // namespace detail

// Kahn's algorithm with a min-heap of ready vertices gives the smallest-id-first topological order.
inline RelaxationTrace dag_relaxation_trace(const WeightedGraph& g, int source) {
    detail::check_source(g, source);
    const std::vector<char> reachable = detail::reachable_from(g, source);
    std::vector<int> pending(g.size(), 0);
    for (int u = 0; u < static_cast<int>(g.size()); ++u) {
        if (reachable[detail::idx(u)] == 0) {
            continue;
        }
        for (const Arc& arc : g[detail::idx(u)]) {
            ++pending[detail::idx(arc.to)];
        }
    }
    RelaxationTrace trace{{}, std::vector<std::int64_t>(g.size(), kUnreachable)};
    trace.dist[detail::idx(source)] = 0;
    std::priority_queue<int, std::vector<int>, std::greater<int>> ready;
    ready.push(source);
    while (!ready.empty()) {
        const int u = ready.top();
        ready.pop();
        for (const Arc& arc : detail::sorted_arcs(g[detail::idx(u)])) {
            trace.order.emplace_back(u, arc.to);
            trace.dist[detail::idx(arc.to)] =
                std::min(trace.dist[detail::idx(arc.to)], trace.dist[detail::idx(u)] + arc.weight);
            if (--pending[detail::idx(arc.to)] == 0) {
                ready.push(arc.to);
            }
        }
    }
    return trace;
}

inline RelaxationTrace dijkstra_trace(const WeightedGraph& g, int source) {
    detail::check_source(g, source);
    RelaxationTrace trace{{}, std::vector<std::int64_t>(g.size(), kUnreachable)};
    trace.dist[detail::idx(source)] = 0;
    std::vector<char> settled(g.size(), 0);
    using Entry = std::pair<std::int64_t, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> frontier;
    frontier.emplace(0, source);
    while (!frontier.empty()) {
        const auto [d, u] = frontier.top();
        frontier.pop();
        if (settled[detail::idx(u)] != 0) {
            continue;
        }
        settled[detail::idx(u)] = 1;
        for (const Arc& arc : detail::sorted_arcs(g[detail::idx(u)])) {
            trace.order.emplace_back(u, arc.to);
            if (d + arc.weight < trace.dist[detail::idx(arc.to)]) {
                trace.dist[detail::idx(arc.to)] = d + arc.weight;
                frontier.emplace(trace.dist[detail::idx(arc.to)], arc.to);
            }
        }
    }
    return trace;
}

// Replays the run: each group of edges names the vertex being extracted, so the check is simply
// that this vertex holds the smallest estimate among the vertices that still have edges to relax.
inline bool is_legal_dijkstra_order(const WeightedGraph& g, int source,
                                    std::span<const RelaxedEdge> order) {
    detail::check_source(g, source);
    const int n = static_cast<int>(g.size());
    std::vector<std::vector<int>> heads(g.size());
    for (int u = 0; u < n; ++u) {
        for (const Arc& arc : g[detail::idx(u)]) {
            heads[detail::idx(u)].push_back(arc.to);
        }
        std::sort(heads[detail::idx(u)].begin(), heads[detail::idx(u)].end());
    }
    std::vector<std::int64_t> dist(g.size(), kUnreachable);
    std::vector<char> settled(g.size(), 0);
    std::set<std::pair<std::int64_t, int>> candidates;
    const auto update = [&](int v, std::int64_t d) {
        if (heads[detail::idx(v)].empty()) {
            dist[detail::idx(v)] = d;
            return;
        }
        if (dist[detail::idx(v)] != kUnreachable) {
            candidates.erase({dist[detail::idx(v)], v});
        }
        dist[detail::idx(v)] = d;
        candidates.emplace(d, v);
    };
    update(source, 0);

    std::size_t position = 0;
    std::vector<int> group;
    while (position < order.size()) {
        const int u = order[position].first;
        if (u < 0 || u >= n || settled[detail::idx(u)] != 0 ||
            dist[detail::idx(u)] == kUnreachable || heads[detail::idx(u)].empty() ||
            candidates.begin()->first < dist[detail::idx(u)]) {
            return false;
        }
        const std::size_t degree = heads[detail::idx(u)].size();
        if (position + degree > order.size()) {
            return false;
        }
        group.clear();
        for (std::size_t i = position; i < position + degree; ++i) {
            if (order[i].first != u) {
                return false;
            }
            group.push_back(order[i].second);
        }
        std::sort(group.begin(), group.end());
        if (group != heads[detail::idx(u)]) {
            return false;
        }
        settled[detail::idx(u)] = 1;
        candidates.erase({dist[detail::idx(u)], u});
        for (const Arc& arc : g[detail::idx(u)]) {
            const std::int64_t candidate = dist[detail::idx(u)] + arc.weight;
            if (settled[detail::idx(arc.to)] == 0 && candidate < dist[detail::idx(arc.to)]) {
                update(arc.to, candidate);
            }
        }
        position += degree;
    }
    return candidates.empty();
}
