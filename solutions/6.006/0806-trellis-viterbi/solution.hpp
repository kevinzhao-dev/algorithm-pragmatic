#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

template <typename NodeCost, typename EdgeCost>
auto min_trellis_cost(std::size_t positions, std::size_t states, NodeCost&& node, EdgeCost&& edge) {
    using Cost = std::invoke_result_t<NodeCost&, std::size_t, std::size_t>;
    if (positions == 0) {
        return Cost{};
    }
    if (states == 0) {
        throw std::invalid_argument("states must be positive");
    }
    std::vector<Cost> current(states);
    std::vector<Cost> next(states);
    for (std::size_t s = 0; s < states; ++s) {
        current[s] = node(0, s);
    }
    for (std::size_t i = 0; i + 1 < positions; ++i) {
        for (std::size_t t = 0; t < states; ++t) {
            Cost best = current[0] + edge(i, 0, t);
            for (std::size_t s = 1; s < states; ++s) {
                best = std::min(best, current[s] + edge(i, s, t));
            }
            next[t] = best + node(i + 1, t);
        }
        std::swap(current, next);
    }
    return *std::ranges::min_element(current);
}

template <typename NodeCost, typename EdgeCost>
std::vector<std::size_t> best_trellis_path(std::size_t positions, std::size_t states,
                                           NodeCost&& node, EdgeCost&& edge) {
    using Cost = std::invoke_result_t<NodeCost&, std::size_t, std::size_t>;
    if (positions == 0) {
        return {};
    }
    if (states == 0) {
        throw std::invalid_argument("states must be positive");
    }
    std::vector<Cost> current(states);
    std::vector<Cost> next(states);
    std::vector<std::size_t> parent((positions - 1) * states);
    for (std::size_t s = 0; s < states; ++s) {
        current[s] = node(0, s);
    }
    for (std::size_t i = 0; i + 1 < positions; ++i) {
        for (std::size_t t = 0; t < states; ++t) {
            Cost best = current[0] + edge(i, 0, t);
            std::size_t best_s = 0;
            for (std::size_t s = 1; s < states; ++s) {
                const Cost candidate = current[s] + edge(i, s, t);
                if (candidate < best) {
                    best = candidate;
                    best_s = s;
                }
            }
            next[t] = best + node(i + 1, t);
            parent[i * states + t] = best_s;
        }
        std::swap(current, next);
    }

    std::vector<std::size_t> path(positions);
    path[positions - 1] = static_cast<std::size_t>(std::ranges::min_element(current) - current.begin());
    for (std::size_t i = positions - 1; i > 0; --i) {
        path[i - 1] = parent[(i - 1) * states + path[i]];
    }
    return path;
}

struct Hmm {
    std::vector<double> start;
    std::vector<std::vector<double>> transition;
    std::vector<std::vector<double>> emission;
};

inline std::vector<std::size_t> viterbi_decode(const Hmm& hmm,
                                               std::span<const std::size_t> observations) {
    const auto cost_of = [](double probability) {
        return probability > 0.0 ? -std::log(probability)
                                 : std::numeric_limits<double>::infinity();
    };
    const auto node = [&](std::size_t i, std::size_t s) {
        const double emit = cost_of(hmm.emission[s][observations[i]]);
        return i == 0 ? emit + cost_of(hmm.start[s]) : emit;
    };
    const auto edge = [&](std::size_t, std::size_t s, std::size_t t) {
        return cost_of(hmm.transition[s][t]);
    };
    return best_trellis_path(observations.size(), hmm.start.size(), node, edge);
}

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
    if (top.size() != bottom.size()) {
        throw std::invalid_argument("rows must have the same length");
    }
    const std::size_t n = top.size();
    constexpr std::int64_t kForbidden = 1'000'000'000'000'000;
    const auto value = [&](std::size_t row, std::size_t column) -> std::int64_t {
        return row == 0 ? top[column] : bottom[column];
    };
    // State x is the set of cells of column x already taken by ships starting in column x - 1.
    const auto node = [&](std::size_t x, std::size_t mask) -> std::int64_t {
        return (x == 0 || x == n) && mask != 0 ? kForbidden : 0;
    };
    const auto edge = [&](std::size_t x, std::size_t covered, std::size_t next_covered) -> std::int64_t {
        // Cells that start horizontal ships must be free, and there must be a column to their right.
        if ((covered & next_covered) != 0 || (x + 1 == n && next_covered != 0)) {
            return kForbidden;
        }
        std::int64_t gain = 0;
        for (std::size_t row = 0; row < 2; ++row) {
            if ((next_covered >> row) & 1) {
                gain += value(row, x) + value(row, x + 1);
            }
        }
        if ((covered | next_covered) == 0) {
            gain += std::max<std::int64_t>(0, value(0, x) + value(1, x));
        }
        return -gain;
    };

    ShipPlan plan{0, {}};
    if (n == 0) {
        return plan;
    }
    const std::vector<std::size_t> masks = best_trellis_path(n + 1, 4, node, edge);
    for (std::size_t x = 0; x < n; ++x) {
        const std::size_t covered = masks[x];
        const std::size_t next_covered = masks[x + 1];
        for (std::size_t row = 0; row < 2; ++row) {
            if ((next_covered >> row) & 1) {
                plan.ships.push_back({row, x, false});
                plan.score += value(row, x) + value(row, x + 1);
            }
        }
        if ((covered | next_covered) == 0 && value(0, x) + value(1, x) > 0) {
            plan.ships.push_back({0, x, true});
            plan.score += value(0, x) + value(1, x);
        }
    }
    return plan;
}
