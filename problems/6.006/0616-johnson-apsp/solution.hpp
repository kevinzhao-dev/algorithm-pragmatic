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

// Stage 1
inline std::optional<std::vector<std::int64_t>> johnson_potentials(int n, std::span<const Edge> edges) {
    PRACTICE_TODO();
}

// Stage 2
inline std::optional<std::vector<std::vector<std::int64_t>>> johnson(int n, std::span<const Edge> edges) {
    PRACTICE_TODO();
}

// Stage 3
class RoutingTable {
public:
    static std::optional<RoutingTable> build(int n, std::span<const Edge> edges) { PRACTICE_TODO(); }

    std::int64_t distance(int from, int to) const { PRACTICE_TODO(); }
    std::optional<int> next_hop(int from, int to) const { PRACTICE_TODO(); }
    std::optional<std::vector<int>> route(int from, int to) const { PRACTICE_TODO(); }

private:
    RoutingTable() = default;
};
