#pragma once

#include <algorithm>
#include <limits>
#include <optional>
#include <queue>
#include <span>
#include <utility>
#include <vector>

using EdgeList = std::span<const std::pair<int, int>>;

struct PartialRouting {
    std::vector<std::vector<int>> routes;
    std::vector<int> cut_edges;
    std::vector<int> free_sources;
};

namespace routes_detail {

// Unit-style Edmonds-Karp on an explicit residual list.
class Flow {
public:
    explicit Flow(int n) : adj_(n) {}

    int add(int u, int v, long long cap) {
        const int arc = static_cast<int>(to_.size());
        adj_[u].push_back(arc);
        to_.push_back(v);
        res_.push_back(cap);
        adj_[v].push_back(arc + 1);
        to_.push_back(u);
        res_.push_back(0);
        return arc;
    }

    long long flow_on(int arc) const { return res_[arc ^ 1]; }

    long long max_flow(int s, int t) {
        long long total = 0;
        while (true) {
            std::vector<int> via(adj_.size(), -1);
            std::queue<int> q;
            q.push(s);
            via[s] = -2;
            while (!q.empty() && via[t] == -1) {
                const int v = q.front();
                q.pop();
                for (int arc : adj_[v]) {
                    if (res_[arc] > 0 && via[to_[arc]] == -1) {
                        via[to_[arc]] = arc;
                        q.push(to_[arc]);
                    }
                }
            }
            if (via[t] == -1) {
                return total;
            }
            long long pushed = std::numeric_limits<long long>::max();
            for (int v = t; v != s; v = to_[via[v] ^ 1]) {
                pushed = std::min(pushed, res_[via[v]]);
            }
            for (int v = t; v != s; v = to_[via[v] ^ 1]) {
                res_[via[v]] -= pushed;
                res_[via[v] ^ 1] += pushed;
            }
            total += pushed;
        }
    }

    std::vector<char> reachable(int s) const {
        std::vector<char> seen(adj_.size(), 0);
        std::queue<int> q;
        q.push(s);
        seen[s] = 1;
        while (!q.empty()) {
            const int v = q.front();
            q.pop();
            for (int arc : adj_[v]) {
                if (res_[arc] > 0 && !seen[to_[arc]]) {
                    seen[to_[arc]] = 1;
                    q.push(to_[arc]);
                }
            }
        }
        return seen;
    }

private:
    std::vector<std::vector<int>> adj_;
    std::vector<int> to_;
    std::vector<long long> res_;
};

// Peels one route per served source from an integral flow (edge_arc: flow arc per edge index).
// next_edges[v] lists edge indices out of v with remaining flow.
inline std::vector<std::vector<int>> extract_routes(
    int n, EdgeList edges, const std::vector<int>& edge_arc, const Flow& flow,
    std::span<const int> sources, const std::vector<int>& source_arc, int target) {
    std::vector<std::vector<int>> outgoing(n);
    for (int id = 0; id < static_cast<int>(edges.size()); ++id) {
        for (long long f = flow.flow_on(edge_arc[id]); f > 0; --f) {
            outgoing[edges[id].first].push_back(id);
        }
    }
    std::vector<std::vector<int>> routes(sources.size());
    for (std::size_t i = 0; i < sources.size(); ++i) {
        if (flow.flow_on(source_arc[i]) == 0) {
            continue;
        }
        // Walk along unused flow edges until the target, then erase loops.
        std::vector<int> walk;
        int v = sources[i];
        while (v != target) {
            const int edge = outgoing[v].back();
            outgoing[v].pop_back();
            walk.push_back(edge);
            v = edges[edge].second;
        }
        std::vector<int> path;
        std::vector<int> position(n, -1);
        position[sources[i]] = 0;
        int cur = sources[i];
        for (int edge : walk) {
            const int to = edges[edge].second;
            if (position[to] != -1) {
                // Back to a vertex on the path: drop the loop.
                while (cur != to) {
                    const int removed = path.back();
                    path.pop_back();
                    position[cur] = -1;
                    cur = edges[removed].first;
                }
                continue;
            }
            path.push_back(edge);
            position[to] = static_cast<int>(path.size());
            cur = to;
        }
        routes[i] = std::move(path);
    }
    return routes;
}

}  // namespace routes_detail

inline PartialRouting best_effort_routes(int n, EdgeList edges, std::span<const int> sources,
                                         int target) {
    const int super_source = n;
    routes_detail::Flow flow(n + 1);
    std::vector<int> edge_arc(edges.size());
    for (int id = 0; id < static_cast<int>(edges.size()); ++id) {
        edge_arc[id] = flow.add(edges[id].first, edges[id].second, 1);
    }
    std::vector<int> source_arc(sources.size());
    for (std::size_t i = 0; i < sources.size(); ++i) {
        source_arc[i] = flow.add(super_source, sources[i], 1);
    }
    flow.max_flow(super_source, target);
    const auto side = flow.reachable(super_source);

    PartialRouting out;
    out.routes = routes_detail::extract_routes(n, edges, edge_arc, flow, sources, source_arc, target);
    for (int id = 0; id < static_cast<int>(edges.size()); ++id) {
        if (side[edges[id].first] && !side[edges[id].second]) {
            out.cut_edges.push_back(id);
        }
    }
    for (int i = 0; i < static_cast<int>(sources.size()); ++i) {
        if (!side[sources[i]]) {
            out.free_sources.push_back(i);
        }
    }
    return out;
}

inline std::optional<std::vector<std::vector<int>>> edge_disjoint_routes(
    int n, EdgeList edges, std::span<const int> sources, int target) {
    auto partial = best_effort_routes(n, edges, sources, target);
    for (const auto& route : partial.routes) {
        if (route.empty()) {
            return std::nullopt;
        }
    }
    return std::move(partial.routes);
}

inline std::optional<std::vector<std::vector<int>>> routes_with_hub_limits(
    int n, EdgeList edges, std::span<const int> sources, int target,
    std::span<const int> hub_limit) {
    // Node splitting: every vertex v becomes v_in = 2v, v_out = 2v + 1.
    const auto in = [](int v) { return 2 * v; };
    const auto out = [](int v) { return 2 * v + 1; };
    const int super_source = 2 * n;
    routes_detail::Flow flow(2 * n + 1);
    for (int v = 0; v < n; ++v) {
        const long long cap =
            hub_limit[v] < 0 ? static_cast<long long>(sources.size()) : hub_limit[v];
        flow.add(in(v), out(v), cap);
    }
    std::vector<int> edge_arc(edges.size());
    for (int id = 0; id < static_cast<int>(edges.size()); ++id) {
        edge_arc[id] = flow.add(out(edges[id].first), in(edges[id].second), 1);
    }
    std::vector<int> source_arc(sources.size());
    for (std::size_t i = 0; i < sources.size(); ++i) {
        source_arc[i] = flow.add(super_source, in(sources[i]), 1);
    }
    const long long total = flow.max_flow(super_source, out(target));
    if (total < static_cast<long long>(sources.size())) {
        return std::nullopt;
    }
    return routes_detail::extract_routes(n, edges, edge_arc, flow, sources, source_arc, target);
}
