#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

struct Edge {
    int from;
    int to;
    std::int64_t weight;
};

inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr std::int64_t kNegativeInfinity = std::numeric_limits<std::int64_t>::min();

// Stages 1 and 2
inline std::vector<std::int64_t> bellman_ford(int n, std::span<const Edge> edges, int source) {
    PRACTICE_TODO();
}

// Stage 3
inline std::optional<std::vector<int>> find_negative_cycle(int n, std::span<const Edge> edges,
                                                           int source) {
    PRACTICE_TODO();
}

// Stage 4
struct Quote {
    int from;
    int to;
    double rate;
};

inline std::optional<std::vector<int>> find_arbitrage(int currencies, std::span<const Quote> quotes) {
    PRACTICE_TODO();
}
