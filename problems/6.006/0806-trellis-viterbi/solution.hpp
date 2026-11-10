#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "practice.hpp"

// Stage 1, tightened in Stage 3
template <typename NodeCost, typename EdgeCost>
auto min_trellis_cost(std::size_t positions, std::size_t states, NodeCost&& node, EdgeCost&& edge) {
    using Cost = std::invoke_result_t<NodeCost&, std::size_t, std::size_t>;
    PRACTICE_TODO();
    return Cost{};
}

// Stage 2
template <typename NodeCost, typename EdgeCost>
std::vector<std::size_t> best_trellis_path(std::size_t positions, std::size_t states,
                                           NodeCost&& node, EdgeCost&& edge) {
    PRACTICE_TODO();
}

// Stage 4
struct Hmm {
    std::vector<double> start;
    std::vector<std::vector<double>> transition;
    std::vector<std::vector<double>> emission;
};

inline std::vector<std::size_t> viterbi_decode(const Hmm& hmm,
                                               std::span<const std::size_t> observations) {
    PRACTICE_TODO();
}

// Stage 5
struct Ship {
    std::size_t row;
    std::size_t column;
    bool vertical;
};

struct ShipPlan {
    std::int64_t score;
    std::vector<Ship> ships;
};

inline ShipPlan best_ship_plan(std::span<const int> top, std::span<const int> bottom) {
    PRACTICE_TODO();
}
