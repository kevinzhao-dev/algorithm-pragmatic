#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
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

inline void check_source(int n, int source) {
    if (source < 0 || source >= n) {
        throw std::out_of_range("no such vertex");
    }
}

// One round over the previous layer only, so a single round can never chain two new edges.
inline bool relax_layer(std::span<const Edge> edges, const std::vector<std::int64_t>& previous,
                        std::vector<std::int64_t>& next) {
    next = previous;
    bool changed = false;
    for (const Edge& e : edges) {
        if (previous[idx(e.from)] != kUnreachable && previous[idx(e.from)] + e.weight < next[idx(e.to)]) {
            next[idx(e.to)] = previous[idx(e.from)] + e.weight;
            changed = true;
        }
    }
    return changed;
}

}  // namespace detail

inline std::vector<std::int64_t> converge_routes(int n, std::span<const Edge> edges, int source) {
    detail::check_source(n, source);
    std::vector<std::int64_t> dist(detail::idx(n), kUnreachable);
    dist[detail::idx(source)] = 0;
    for (int round = 0; round < n; ++round) {
        bool changed = false;
        for (const Edge& e : edges) {
            if (dist[detail::idx(e.from)] != kUnreachable &&
                dist[detail::idx(e.from)] + e.weight < dist[detail::idx(e.to)]) {
                dist[detail::idx(e.to)] = dist[detail::idx(e.from)] + e.weight;
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }
    return dist;
}

inline std::vector<std::int64_t> hop_limited_routes(int n, std::span<const Edge> edges, int source,
                                                    int hops) {
    detail::check_source(n, source);
    std::vector<std::int64_t> current(detail::idx(n), kUnreachable);
    current[detail::idx(source)] = 0;
    std::vector<std::int64_t> next;
    for (int round = 0; round < hops; ++round) {
        const bool changed = detail::relax_layer(edges, current, next);
        current.swap(next);
        if (!changed) {
            break;
        }
    }
    return current;
}

inline int convergence_hops(int n, std::span<const Edge> edges, int source) {
    detail::check_source(n, source);
    std::vector<std::int64_t> current(detail::idx(n), kUnreachable);
    current[detail::idx(source)] = 0;
    std::vector<std::int64_t> next;
    int rounds = 0;
    while (detail::relax_layer(edges, current, next)) {
        current.swap(next);
        ++rounds;
    }
    return rounds;
}
