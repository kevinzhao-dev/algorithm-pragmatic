#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

struct Edge {
    int from;
    int to;
    std::int64_t weight;
};

inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr std::int64_t kMinusInfinity = std::numeric_limits<std::int64_t>::min();

inline std::vector<std::int64_t> shortest_within_edges(int vertices, std::span<const Edge> edges,
                                                       int source, int max_edges) {
    std::vector<std::int64_t> best(static_cast<std::size_t>(vertices), kUnreachable);
    best[static_cast<std::size_t>(source)] = 0;
    std::vector<std::int64_t> next = best;
    for (int round = 0; round < max_edges; ++round) {
        // Every relaxation reads `best`, so one round can only extend paths by a single edge.
        next = best;
        bool changed = false;
        for (const Edge& edge : edges) {
            const auto from = static_cast<std::size_t>(edge.from);
            const auto to = static_cast<std::size_t>(edge.to);
            if (best[from] != kUnreachable && best[from] + edge.weight < next[to]) {
                next[to] = best[from] + edge.weight;
                changed = true;
            }
        }
        std::swap(best, next);
        if (!changed) {
            break;
        }
    }
    return best;
}

class AllPairs {
public:
    AllPairs(int vertices, std::span<const Edge> edges)
        : n_(static_cast<std::size_t>(vertices)),
          distance_(n_ * n_, kUnreachable),
          next_(n_ * n_, -1) {
        for (std::size_t v = 0; v < n_; ++v) {
            at(distance_, v, v) = 0;
            at(next_, v, v) = static_cast<int>(v);
        }
        for (const Edge& edge : edges) {
            const auto from = static_cast<std::size_t>(edge.from);
            const auto to = static_cast<std::size_t>(edge.to);
            if (from != to && edge.weight < at(distance_, from, to)) {
                at(distance_, from, to) = edge.weight;
                at(next_, from, to) = edge.to;
            }
            if (from == to && edge.weight < at(distance_, from, to)) {
                at(distance_, from, to) = edge.weight;  // a negative self-loop is a negative cycle
            }
        }

        for (std::size_t k = 0; k < n_; ++k) {
            for (std::size_t i = 0; i < n_; ++i) {
                const std::int64_t to_k = at(distance_, i, k);
                if (to_k == kUnreachable) {
                    continue;
                }
                for (std::size_t j = 0; j < n_; ++j) {
                    const std::int64_t from_k = at(distance_, k, j);
                    if (from_k == kUnreachable) {
                        continue;
                    }
                    const std::int64_t through = std::max(to_k + from_k, kFloor);
                    if (through < at(distance_, i, j)) {
                        at(distance_, i, j) = through;
                        at(next_, i, j) = at(next_, i, k);
                    }
                }
            }
        }

        for (std::size_t v = 0; v < n_; ++v) {
            negative_cycle_ = negative_cycle_ || at(distance_, v, v) < 0;
        }
        if (negative_cycle_) {
            mark_negative_cycle_pairs();
        }
    }

    std::int64_t distance(int from, int to) const {
        return at(distance_, static_cast<std::size_t>(from), static_cast<std::size_t>(to));
    }

    std::vector<int> path(int from, int to) const {
        if (distance(from, to) == kUnreachable || distance(from, to) == kMinusInfinity) {
            return {};
        }
        std::vector<int> vertices{from};
        for (int at_vertex = from; at_vertex != to;) {
            at_vertex = at(next_, static_cast<std::size_t>(at_vertex), static_cast<std::size_t>(to));
            vertices.push_back(at_vertex);
        }
        return vertices;
    }

    bool has_negative_cycle() const { return negative_cycle_; }

private:
    // Clamp for the doubling that negative cycles cause; twice this still fits in 64 bits.
    static constexpr std::int64_t kFloor = std::numeric_limits<std::int64_t>::min() / 2;

    template <typename T>
    T& at(std::vector<T>& table, std::size_t from, std::size_t to) const {
        return table[from * n_ + to];
    }
    template <typename T>
    const T& at(const std::vector<T>& table, std::size_t from, std::size_t to) const {
        return table[from * n_ + to];
    }

    void mark_negative_cycle_pairs() {
        std::vector<char> on_negative_cycle(n_, 0);
        for (std::size_t t = 0; t < n_; ++t) {
            on_negative_cycle[t] = at(distance_, t, t) < 0 ? 1 : 0;
        }
        std::vector<std::int64_t> marked = distance_;
        for (std::size_t t = 0; t < n_; ++t) {
            if (!on_negative_cycle[t]) {
                continue;
            }
            for (std::size_t i = 0; i < n_; ++i) {
                if (at(distance_, i, t) == kUnreachable) {
                    continue;
                }
                for (std::size_t j = 0; j < n_; ++j) {
                    if (at(distance_, t, j) != kUnreachable) {
                        at(marked, i, j) = kMinusInfinity;
                    }
                }
            }
        }
        distance_ = std::move(marked);
    }

    std::size_t n_;
    std::vector<std::int64_t> distance_;
    std::vector<int> next_;
    bool negative_cycle_ = false;
};
