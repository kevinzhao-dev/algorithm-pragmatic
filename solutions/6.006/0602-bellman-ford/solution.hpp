#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

struct Edge {
    int from;
    int to;
    std::int64_t weight;
};

inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr std::int64_t kNegativeInfinity = std::numeric_limits<std::int64_t>::min();

struct Quote {
    int from;
    int to;
    double rate;
};

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

inline void check_source(int n, int source) {
    if (source < 0 || source >= n) {
        throw std::out_of_range("no such vertex");
    }
}

// Follows predecessor pointers from a vertex relaxed in the final round. Stepping V times is
// guaranteed to land on the cycle; from there, one more trip around collects it in order.
inline std::vector<int> extract_cycle(const std::vector<int>& parent, int witness, int n) {
    for (int step = 0; step < n; ++step) {
        witness = parent[idx(witness)];
    }
    std::vector<int> cycle;
    int at = witness;
    do {
        cycle.push_back(at);
        at = parent[idx(at)];
    } while (at != witness);
    std::reverse(cycle.begin(), cycle.end());
    return cycle;
}

}  // namespace detail

inline std::vector<std::int64_t> bellman_ford(int n, std::span<const Edge> edges, int source) {
    detail::check_source(n, source);
    std::vector<std::int64_t> dist(detail::idx(n), kUnreachable);
    dist[detail::idx(source)] = 0;
    for (int round = 0; round + 1 < n; ++round) {
        bool changed = false;
        for (const Edge& e : edges) {
            if (dist[detail::idx(e.from)] != kUnreachable &&
                dist[detail::idx(e.from)] + e.weight < dist[detail::idx(e.to)]) {
                dist[detail::idx(e.to)] = dist[detail::idx(e.from)] + e.weight;
                changed = true;
            }
        }
        if (!changed) {
            return dist;
        }
    }

    // A vertex still improvable after V-1 rounds is a witness; everything it reaches is -infinity.
    std::vector<char> infected(detail::idx(n), 0);
    std::vector<int> stack;
    for (const Edge& e : edges) {
        if (dist[detail::idx(e.from)] != kUnreachable &&
            dist[detail::idx(e.from)] + e.weight < dist[detail::idx(e.to)] &&
            infected[detail::idx(e.to)] == 0) {
            infected[detail::idx(e.to)] = 1;
            stack.push_back(e.to);
        }
    }
    if (!stack.empty()) {
        std::vector<std::vector<int>> adjacency(detail::idx(n));
        for (const Edge& e : edges) {
            adjacency[detail::idx(e.from)].push_back(e.to);
        }
        while (!stack.empty()) {
            const int u = stack.back();
            stack.pop_back();
            for (const int v : adjacency[detail::idx(u)]) {
                if (infected[detail::idx(v)] == 0) {
                    infected[detail::idx(v)] = 1;
                    stack.push_back(v);
                }
            }
        }
        for (int v = 0; v < n; ++v) {
            if (infected[detail::idx(v)] != 0) {
                dist[detail::idx(v)] = kNegativeInfinity;
            }
        }
    }
    return dist;
}

inline std::optional<std::vector<int>> find_negative_cycle(int n, std::span<const Edge> edges,
                                                           int source) {
    detail::check_source(n, source);
    std::vector<std::int64_t> dist(detail::idx(n), kUnreachable);
    std::vector<int> parent(detail::idx(n), -1);
    dist[detail::idx(source)] = 0;
    int witness = -1;
    for (int round = 0; round < n; ++round) {
        witness = -1;
        for (const Edge& e : edges) {
            if (dist[detail::idx(e.from)] != kUnreachable &&
                dist[detail::idx(e.from)] + e.weight < dist[detail::idx(e.to)]) {
                dist[detail::idx(e.to)] = dist[detail::idx(e.from)] + e.weight;
                parent[detail::idx(e.to)] = e.from;
                witness = e.to;
            }
        }
        if (witness == -1) {
            return std::nullopt;
        }
    }
    return detail::extract_cycle(parent, witness, n);
}

inline std::optional<std::vector<int>> find_arbitrage(int currencies, std::span<const Quote> quotes) {
    constexpr double kEpsilon = 1e-12;  // absorbs rounding error in sums of logarithms
    std::vector<double> weights;
    weights.reserve(quotes.size());
    for (const Quote& q : quotes) {
        if (q.rate <= 0.0) {
            throw std::invalid_argument("rates must be positive");
        }
        weights.push_back(-std::log(q.rate));
    }
    // Starting every currency at distance zero is the supernode trick without the extra vertex.
    std::vector<double> dist(detail::idx(currencies), 0.0);
    std::vector<int> parent(detail::idx(currencies), -1);
    int witness = -1;
    for (int round = 0; round < currencies; ++round) {
        witness = -1;
        for (std::size_t i = 0; i < quotes.size(); ++i) {
            const Quote& q = quotes[i];
            if (dist[detail::idx(q.from)] + weights[i] + kEpsilon < dist[detail::idx(q.to)]) {
                dist[detail::idx(q.to)] = dist[detail::idx(q.from)] + weights[i];
                parent[detail::idx(q.to)] = q.from;
                witness = q.to;
            }
        }
        if (witness == -1) {
            return std::nullopt;
        }
    }
    return detail::extract_cycle(parent, witness, currencies);
}
