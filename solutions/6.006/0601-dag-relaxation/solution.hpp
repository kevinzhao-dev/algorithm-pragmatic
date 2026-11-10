#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

struct Arc {
    int to;
    std::int64_t weight;
};

using WeightedGraph = std::vector<std::vector<Arc>>;

inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr int kNoParent = -1;

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

inline void check_vertex(const WeightedGraph& g, int v) {
    if (v < 0 || idx(v) >= g.size()) {
        throw std::out_of_range("no such vertex");
    }
}

// Kahn's algorithm: an iterative topological sort that doubles as cycle detection.
inline std::vector<int> topological_order(const WeightedGraph& g) {
    std::vector<int> indegree(g.size(), 0);
    for (const auto& arcs : g) {
        for (const Arc& arc : arcs) {
            ++indegree[idx(arc.to)];
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
        for (const Arc& arc : g[idx(order[head])]) {
            if (--indegree[idx(arc.to)] == 0) {
                order.push_back(arc.to);
            }
        }
    }
    if (order.size() != g.size()) {
        throw std::invalid_argument("graph has a directed cycle");
    }
    return order;
}

}  // namespace detail

inline std::vector<std::int64_t> dag_shortest_paths(const WeightedGraph& g, int source) {
    detail::check_vertex(g, source);
    const std::vector<int> order = detail::topological_order(g);
    std::vector<std::int64_t> dist(g.size(), kUnreachable);
    dist[detail::idx(source)] = 0;
    for (const int u : order) {
        if (dist[detail::idx(u)] == kUnreachable) {
            continue;
        }
        for (const Arc& arc : g[detail::idx(u)]) {
            dist[detail::idx(arc.to)] = std::min(dist[detail::idx(arc.to)], dist[detail::idx(u)] + arc.weight);
        }
    }
    return dist;
}

// BFS restricted to tight edges. Any shortest path consists of tight edges, so everything
// reachable is found, and because BFS only claims unseen vertices a zero-weight cycle cannot
// turn the parent pointers into a loop.
inline std::vector<int> shortest_path_tree(const WeightedGraph& g, int source,
                                           std::span<const std::int64_t> dist) {
    detail::check_vertex(g, source);
    std::vector<int> parent(g.size(), kNoParent);
    std::vector<char> seen(g.size(), 0);
    std::vector<int> queue{source};
    seen[detail::idx(source)] = 1;
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const int u = queue[head];
        for (const Arc& arc : g[detail::idx(u)]) {
            if (seen[detail::idx(arc.to)] == 0 && dist[detail::idx(u)] + arc.weight == dist[detail::idx(arc.to)]) {
                seen[detail::idx(arc.to)] = 1;
                parent[detail::idx(arc.to)] = u;
                queue.push_back(arc.to);
            }
        }
    }
    return parent;
}

struct Job {
    std::int64_t duration;
    std::vector<int> depends_on;
};

struct PipelinePlan {
    std::int64_t makespan;
    std::vector<std::int64_t> earliest_start;
    std::vector<std::int64_t> slack;
    std::vector<int> critical_path;
};

inline PipelinePlan plan_pipeline(std::span<const Job> jobs) {
    const auto n = jobs.size();
    WeightedGraph successors(n);
    for (std::size_t job = 0; job < n; ++job) {
        for (const int dependency : jobs[job].depends_on) {
            if (dependency < 0 || detail::idx(dependency) >= n) {
                throw std::out_of_range("no such job");
            }
            successors[detail::idx(dependency)].push_back({static_cast<int>(job), 0});
        }
    }
    const std::vector<int> order = detail::topological_order(successors);

    PipelinePlan plan{0, std::vector<std::int64_t>(n, 0), std::vector<std::int64_t>(n, 0), {}};
    std::vector<int> latest_predecessor(n, kNoParent);
    int last = kNoParent;
    for (const int job : order) {
        const std::int64_t finish = plan.earliest_start[detail::idx(job)] + jobs[detail::idx(job)].duration;
        if (last == kNoParent || finish > plan.makespan) {
            plan.makespan = finish;
            last = job;
        }
        for (const Arc& edge : successors[detail::idx(job)]) {
            if (finish >= plan.earliest_start[detail::idx(edge.to)]) {
                plan.earliest_start[detail::idx(edge.to)] = finish;
                latest_predecessor[detail::idx(edge.to)] = job;
            }
        }
    }

    std::vector<std::int64_t> latest_finish(n, plan.makespan);
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        const int job = *it;
        const std::int64_t latest_start = latest_finish[detail::idx(job)] - jobs[detail::idx(job)].duration;
        plan.slack[detail::idx(job)] = latest_start - plan.earliest_start[detail::idx(job)];
        for (const int dependency : jobs[detail::idx(job)].depends_on) {
            latest_finish[detail::idx(dependency)] = std::min(latest_finish[detail::idx(dependency)], latest_start);
        }
    }

    for (int job = last; job != kNoParent; job = latest_predecessor[detail::idx(job)]) {
        plan.critical_path.push_back(job);
    }
    std::reverse(plan.critical_path.begin(), plan.critical_path.end());
    return plan;
}
