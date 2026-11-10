#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

struct Pipe {
    int a;
    int b;
    std::int64_t length;
};

struct HeadDrop {
    int junction;
    std::int64_t new_head;
};

struct PipeNetwork {
    std::vector<std::int64_t> head;
    std::vector<Pipe> pipes;
    std::vector<HeadDrop> drops;
    int valve;
};

struct DriftPlan {
    std::int64_t length;
    std::vector<int> route;
    std::optional<std::size_t> trigger;
};

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }
inline constexpr std::int64_t kInfinity = std::numeric_limits<std::int64_t>::max();

struct StateArc {
    int to;
    std::int64_t length;
};

struct Search {
    int junctions;
    std::vector<std::int64_t> dist;
    std::vector<int> parent;
};

// Two stacked copies of the network: state v is "at junction v, valve untouched", state n + v is
// "at junction v, valve already fired". Within a copy heads strictly drop along every arc, and the
// only bridge is valve -> n + valve, so the combined graph is a DAG and one relaxation pass over a
// topological order suffices.
inline Search search(const PipeNetwork& network, int source, int sink) {
    const int n = static_cast<int>(network.head.size());
    if (source < 0 || source >= n || sink < 0 || sink >= n) {
        throw std::out_of_range("no such junction");
    }
    std::vector<std::int64_t> after = network.head;
    for (const HeadDrop& drop : network.drops) {
        after[idx(drop.junction)] = drop.new_head;
    }

    std::vector<std::vector<StateArc>> arcs(2 * idx(n));
    std::vector<int> pending(2 * idx(n), 0);
    const auto add_arc = [&](int from, int to, std::int64_t length) {
        arcs[idx(from)].push_back({to, length});
        ++pending[idx(to)];
    };
    for (const Pipe& pipe : network.pipes) {
        for (int layer = 0; layer < 2; ++layer) {
            const auto& heads = layer == 0 ? network.head : after;
            const int offset = layer * n;
            if (heads[idx(pipe.a)] > heads[idx(pipe.b)]) {
                add_arc(offset + pipe.a, offset + pipe.b, pipe.length);
            } else if (heads[idx(pipe.b)] > heads[idx(pipe.a)]) {
                add_arc(offset + pipe.b, offset + pipe.a, pipe.length);
            }
        }
    }
    add_arc(network.valve, n + network.valve, 0);

    Search out{n, std::vector<std::int64_t>(2 * idx(n), kInfinity), std::vector<int>(2 * idx(n), -1)};
    out.dist[idx(source)] = 0;
    std::vector<int> order;
    order.reserve(2 * idx(n));
    for (int state = 0; state < 2 * n; ++state) {
        if (pending[idx(state)] == 0) {
            order.push_back(state);
        }
    }
    for (std::size_t head = 0; head < order.size(); ++head) {
        const int state = order[head];
        for (const StateArc& arc : arcs[idx(state)]) {
            if (out.dist[idx(state)] != kInfinity &&
                out.dist[idx(state)] + arc.length < out.dist[idx(arc.to)]) {
                out.dist[idx(arc.to)] = out.dist[idx(state)] + arc.length;
                out.parent[idx(arc.to)] = state;
            }
            if (--pending[idx(arc.to)] == 0) {
                order.push_back(arc.to);
            }
        }
    }
    return out;
}

}  // namespace detail

inline std::optional<std::int64_t> shortest_drift(const PipeNetwork& network, int source, int sink) {
    const auto found = detail::search(network, source, sink);
    const std::int64_t best = std::min(found.dist[detail::idx(sink)],
                                       found.dist[detail::idx(found.junctions + sink)]);
    if (best == detail::kInfinity) {
        return std::nullopt;
    }
    return best;
}

inline std::optional<DriftPlan> plan_drift(const PipeNetwork& network, int source, int sink) {
    const auto found = detail::search(network, source, sink);
    const int n = found.junctions;
    int state = found.dist[detail::idx(sink)] <= found.dist[detail::idx(n + sink)] ? sink : n + sink;
    if (found.dist[detail::idx(state)] == detail::kInfinity) {
        return std::nullopt;
    }

    DriftPlan plan{found.dist[detail::idx(state)], {}, std::nullopt};
    std::vector<int> states;
    for (int s = state; s != -1; s = found.parent[detail::idx(s)]) {
        states.push_back(s);
    }
    std::reverse(states.begin(), states.end());
    for (std::size_t i = 0; i < states.size(); ++i) {
        const int junction = states[i] % n;
        if (!plan.route.empty() && plan.route.back() == junction) {
            plan.trigger = plan.route.size() - 1;
        } else {
            plan.route.push_back(junction);
        }
    }
    return plan;
}
