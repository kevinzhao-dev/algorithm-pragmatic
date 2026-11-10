#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

#include "07_heap_meter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using NodeCosts = std::vector<std::vector<std::int64_t>>;
using EdgeCosts = std::vector<std::vector<std::vector<std::int64_t>>>;

std::int64_t path_cost(const NodeCosts& node, const EdgeCosts& edge,
                       const std::vector<std::size_t>& path) {
    std::int64_t total = 0;
    for (std::size_t i = 0; i < path.size(); ++i) {
        total += node[i][path[i]];
        if (i + 1 < path.size()) {
            total += edge[i][path[i]][path[i + 1]];
        }
    }
    return total;
}

// Enumerates every assignment of a state to every position.
std::int64_t exhaustive_trellis(const NodeCosts& node, const EdgeCosts& edge, std::size_t states) {
    const std::size_t positions = node.size();
    std::vector<std::size_t> path(positions, 0);
    std::int64_t best = std::numeric_limits<std::int64_t>::max();
    while (true) {
        best = std::min(best, path_cost(node, edge, path));
        std::size_t i = 0;
        while (i < positions && ++path[i] == states) {
            path[i] = 0;
            ++i;
        }
        if (i == positions) {
            return best;
        }
    }
}

struct RandomTrellis {
    NodeCosts node;
    EdgeCosts edge;
    std::size_t states;
};

RandomTrellis random_trellis(Rng& rng, std::size_t positions, std::size_t states) {
    RandomTrellis t{NodeCosts(positions, std::vector<std::int64_t>(states)),
                    EdgeCosts(positions == 0 ? 0 : positions - 1,
                              std::vector<std::vector<std::int64_t>>(states, std::vector<std::int64_t>(states))),
                    states};
    for (auto& row : t.node) {
        for (auto& cost : row) {
            cost = rng.uniform(-9, 9);
        }
    }
    for (auto& layer : t.edge) {
        for (auto& row : layer) {
            for (auto& cost : row) {
                cost = rng.uniform(-9, 9);
            }
        }
    }
    return t;
}

double log_probability(const Hmm& hmm, std::span<const std::size_t> observations,
                       const std::vector<std::size_t>& states) {
    double total = std::log(hmm.start[states[0]]) + std::log(hmm.emission[states[0]][observations[0]]);
    for (std::size_t i = 1; i < states.size(); ++i) {
        total += std::log(hmm.transition[states[i - 1]][states[i]]);
        total += std::log(hmm.emission[states[i]][observations[i]]);
    }
    return total;
}

Hmm random_hmm(Rng& rng, std::size_t states, std::size_t symbols) {
    const auto stochastic = [&](std::size_t size) {
        std::vector<double> row(size);
        double total = 0;
        for (double& p : row) {
            p = 0.1 + rng.real();
            total += p;
        }
        for (double& p : row) {
            p /= total;
        }
        return row;
    };
    Hmm hmm;
    hmm.start = stochastic(states);
    hmm.transition.resize(states);
    hmm.emission.resize(states);
    for (std::size_t s = 0; s < states; ++s) {
        hmm.transition[s] = stochastic(states);
        hmm.emission[s] = stochastic(symbols);
    }
    return hmm;
}

// Best log-probability, by checking every state sequence.
double exhaustive_viterbi(const Hmm& hmm, const std::vector<std::size_t>& observations) {
    const std::size_t states = hmm.start.size();
    std::vector<std::size_t> path(observations.size(), 0);
    double best = -std::numeric_limits<double>::infinity();
    while (true) {
        best = std::max(best, log_probability(hmm, observations, path));
        std::size_t i = 0;
        while (i < path.size() && ++path[i] == states) {
            path[i] = 0;
            ++i;
        }
        if (i == path.size()) {
            return best;
        }
    }
}

std::vector<int> random_row(Rng& rng, std::size_t count) {
    return rng.ints(count, -9, 12);
}

// Tries every way to place ships, column by column.
std::int64_t brute_ships(std::span<const int> top, std::span<const int> bottom, std::size_t column,
                         unsigned covered_now, unsigned covered_next) {
    if (column == top.size()) {
        return 0;
    }
    if (covered_now == 3) {
        return brute_ships(top, bottom, column + 1, covered_next, 0);
    }
    const unsigned row = (covered_now & 1) != 0 ? 1 : 0;
    std::int64_t best = brute_ships(top, bottom, column, covered_now | (1u << row), covered_next);  // leave empty
    if ((covered_now & 3) == 0) {
        const std::int64_t gain = top[column] + bottom[column];
        best = std::max(best, gain + brute_ships(top, bottom, column + 1, covered_next, 0));
    }
    if (column + 1 < top.size() && ((covered_next >> row) & 1) == 0) {
        const int here = row == 0 ? top[column] : bottom[column];
        const int there = row == 0 ? top[column + 1] : bottom[column + 1];
        best = std::max(best, here + there +
                                  brute_ships(top, bottom, column, covered_now | (1u << row),
                                              covered_next | (1u << row)));
    }
    return best;
}

// Validates a ship layout and returns its score.
std::int64_t score_ships(std::span<const int> top, std::span<const int> bottom,
                         const std::vector<Ship>& ships) {
    std::vector<int> used(top.size() * 2, 0);
    std::int64_t score = 0;
    const auto occupy = [&](std::size_t row, std::size_t column) -> std::int64_t {
        REQUIRE(row < 2);
        REQUIRE(column < top.size());
        REQUIRE(used[row * top.size() + column] == 0);
        used[row * top.size() + column] = 1;
        return row == 0 ? top[column] : bottom[column];
    };
    for (const Ship& ship : ships) {
        if (ship.vertical) {
            REQUIRE_EQ(ship.row, 0);
            score += occupy(0, ship.column);
            score += occupy(1, ship.column);
        } else {
            score += occupy(ship.row, ship.column);
            score += occupy(ship.row, ship.column + 1);
        }
    }
    return score;
}

}  // namespace

TEST(1, "zero positions cost nothing and call nothing") {
    int calls = 0;
    const auto node = [&](std::size_t, std::size_t) -> std::int64_t {
        ++calls;
        return 1;
    };
    const auto edge = [&](std::size_t, std::size_t, std::size_t) -> std::int64_t { return 1; };
    CHECK_EQ(min_trellis_cost(0, 3, node, edge), 0);
    CHECK_EQ(calls, 0);
}

TEST(1, "zero states with some positions are rejected") {
    const auto node = [](std::size_t, std::size_t) -> std::int64_t { return 0; };
    const auto edge = [](std::size_t, std::size_t, std::size_t) -> std::int64_t { return 0; };
    CHECK_THROWS_AS(min_trellis_cost(2, 0, node, edge), std::invalid_argument);
}

TEST(1, "one position picks the cheapest node") {
    const auto node = [](std::size_t, std::size_t s) -> std::int64_t { return s == 1 ? -5 : 3; };
    const auto edge = [](std::size_t, std::size_t, std::size_t) -> std::int64_t { return 0; };
    CHECK_EQ(min_trellis_cost(1, 3, node, edge), -5);
}

TEST(1, "the cost type follows the callbacks") {
    const auto node = [](std::size_t, std::size_t) { return 0.5; };
    const auto edge = [](std::size_t, std::size_t, std::size_t) { return 0.25; };
    CHECK_NEAR(min_trellis_cost(3, 2, node, edge), 2.0, 1e-9);
}

TEST(1, "a transition cost can outweigh a locally cheaper state") {
    const NodeCosts node{{0, 5}, {0, 5}};
    const EdgeCosts edge{{{100, 100}, {0, 0}}};
    // Staying on the node-cheap state 0 costs 100 on the edge.
    const auto n = [&](std::size_t i, std::size_t s) { return node[i][s]; };
    const auto e = [&](std::size_t i, std::size_t s, std::size_t t) { return edge[i][s][t]; };
    CHECK_EQ(min_trellis_cost(2, 2, n, e), 5);
}

TEST(1, "random trellises match exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const auto positions = static_cast<std::size_t>(rng.uniform(1, 6));
        const auto states = static_cast<std::size_t>(rng.uniform(1, 3));
        const auto t = random_trellis(rng, positions, states);
        const auto node = [&](std::size_t i, std::size_t s) { return t.node[i][s]; };
        const auto edge = [&](std::size_t i, std::size_t s, std::size_t u) { return t.edge[i][s][u]; };
        INFO("trial ", trial);
        REQUIRE_EQ(min_trellis_cost(positions, states, node, edge),
                   exhaustive_trellis(t.node, t.edge, states));
    }
}

TEST(1, "every edge cost is evaluated at most once") {
    Rng rng;
    const auto t = random_trellis(rng, 20, 6);
    std::size_t calls = 0;
    const auto node = [&](std::size_t i, std::size_t s) { return t.node[i][s]; };
    const auto edge = [&](std::size_t i, std::size_t s, std::size_t u) {
        ++calls;
        return t.edge[i][s][u];
    };
    min_trellis_cost(std::size_t{20}, std::size_t{6}, node, edge);
    CHECK_LE(calls, 19 * 6 * 6);
}

PERF(1, "a hundred thousand positions and thirty states", 1500) {
    Rng rng;
    const auto values = rng.ints(1024, -50, 50);
    const auto node = [&](std::size_t i, std::size_t s) -> std::int64_t {
        return values[(i * 31 + s * 17) & 1023];
    };
    const auto edge = [&](std::size_t i, std::size_t s, std::size_t t) -> std::int64_t {
        return values[(i * 7 + s * 13 + t * 3) & 1023];
    };
    CHECK_LT(min_trellis_cost(std::size_t{100'000}, std::size_t{30}, node, edge), 0);
}

TEST(2, "zero positions give an empty path") {
    const auto node = [](std::size_t, std::size_t) -> std::int64_t { return 1; };
    const auto edge = [](std::size_t, std::size_t, std::size_t) -> std::int64_t { return 1; };
    CHECK(best_trellis_path(0, 2, node, edge).empty());
}

TEST(2, "paths have one state per position and reach the optimum") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const auto positions = static_cast<std::size_t>(rng.uniform(1, 6));
        const auto states = static_cast<std::size_t>(rng.uniform(1, 3));
        const auto t = random_trellis(rng, positions, states);
        const auto node = [&](std::size_t i, std::size_t s) { return t.node[i][s]; };
        const auto edge = [&](std::size_t i, std::size_t s, std::size_t u) { return t.edge[i][s][u]; };
        INFO("trial ", trial);
        const auto path = best_trellis_path(positions, states, node, edge);
        REQUIRE_EQ(path.size(), positions);
        for (const std::size_t s : path) {
            REQUIRE(s < states);
        }
        REQUIRE_EQ(path_cost(t.node, t.edge, path), exhaustive_trellis(t.node, t.edge, states));
    }
}

TEST(2, "the path also evaluates each edge at most once") {
    Rng rng;
    const auto t = random_trellis(rng, 20, 6);
    std::size_t calls = 0;
    const auto node = [&](std::size_t i, std::size_t s) { return t.node[i][s]; };
    const auto edge = [&](std::size_t i, std::size_t s, std::size_t u) {
        ++calls;
        return t.edge[i][s][u];
    };
    CHECK_EQ(best_trellis_path(std::size_t{20}, std::size_t{6}, node, edge).size(), 20);
    CHECK_LE(calls, 19 * 6 * 6);
}

TEST(3, "min_trellis_cost keeps memory independent of the number of positions") {
    const auto node = [](std::size_t i, std::size_t s) -> std::int64_t {
        return static_cast<std::int64_t>((i * 3 + s * 7) % 11) - 5;
    };
    const auto edge = [](std::size_t i, std::size_t s, std::size_t t) -> std::int64_t {
        return static_cast<std::int64_t>((i + s * 2 + t * 5) % 7) - 3;
    };
    heap_meter::Scope scope;
    const std::int64_t cost = min_trellis_cost(std::size_t{200'000}, std::size_t{4}, node, edge);
    CHECK_LE(scope.peak_bytes(), 4096);
    CHECK_LT(cost, 0);
}

TEST(3, "rolling keeps the same answers") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const auto positions = static_cast<std::size_t>(rng.uniform(1, 6));
        const auto states = static_cast<std::size_t>(rng.uniform(1, 3));
        const auto t = random_trellis(rng, positions, states);
        const auto node = [&](std::size_t i, std::size_t s) { return t.node[i][s]; };
        const auto edge = [&](std::size_t i, std::size_t s, std::size_t u) { return t.edge[i][s][u]; };
        INFO("trial ", trial);
        REQUIRE_EQ(min_trellis_cost(positions, states, node, edge),
                   exhaustive_trellis(t.node, t.edge, states));
    }
}

TEST(4, "no observations give no states") {
    Hmm hmm{{1.0}, {{1.0}}, {{1.0}}};
    CHECK(viterbi_decode(hmm, {}).empty());
}

TEST(4, "a single observation picks the likeliest start and emission") {
    Hmm hmm{{0.6, 0.4}, {{0.5, 0.5}, {0.5, 0.5}}, {{0.9, 0.1}, {0.2, 0.8}}};
    const std::vector<std::size_t> seen{1};
    CHECK_EQ(viterbi_decode(hmm, seen), std::vector<std::size_t>{1});
}

TEST(4, "zero-probability transitions are never used") {
    Hmm hmm{{1.0, 0.0}, {{1.0, 0.0}, {1.0, 0.0}}, {{0.1, 0.9}, {0.99, 0.01}}};
    const std::vector<std::size_t> seen{0, 0, 1};
    CHECK_EQ(viterbi_decode(hmm, seen), std::vector<std::size_t>(3, 0));
}

TEST(4, "random models match checking every state sequence") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const auto states = static_cast<std::size_t>(rng.uniform(1, 3));
        const auto symbols = static_cast<std::size_t>(rng.uniform(1, 3));
        const auto length = static_cast<std::size_t>(rng.uniform(1, 7));
        const Hmm hmm = random_hmm(rng, states, symbols);
        std::vector<std::size_t> observations(length);
        for (std::size_t& o : observations) {
            o = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(symbols) - 1));
        }
        INFO("trial ", trial);
        const auto path = viterbi_decode(hmm, observations);
        REQUIRE_EQ(path.size(), length);
        REQUIRE(std::abs(log_probability(hmm, observations, path) -
                         exhaustive_viterbi(hmm, observations)) < 1e-9);
    }
}

TEST(4, "long sequences do not underflow to a meaningless answer") {
    Rng rng;
    const std::size_t states = 2;
    const Hmm hmm = random_hmm(rng, states, 2);
    std::vector<std::size_t> observations(5000);
    for (std::size_t& o : observations) {
        o = static_cast<std::size_t>(rng.uniform(0, 1));
    }
    const auto path = viterbi_decode(hmm, observations);
    REQUIRE_EQ(path.size(), observations.size());

    // A log-space reference recursion over the two states.
    std::vector<double> value(states);
    for (std::size_t s = 0; s < states; ++s) {
        value[s] = std::log(hmm.start[s]) + std::log(hmm.emission[s][observations[0]]);
    }
    for (std::size_t i = 1; i < observations.size(); ++i) {
        std::vector<double> next(states);
        for (std::size_t t = 0; t < states; ++t) {
            next[t] = -std::numeric_limits<double>::infinity();
            for (std::size_t s = 0; s < states; ++s) {
                next[t] = std::max(next[t], value[s] + std::log(hmm.transition[s][t]));
            }
            next[t] += std::log(hmm.emission[t][observations[i]]);
        }
        value = next;
    }
    const double best = *std::ranges::max_element(value);
    CHECK_NEAR(log_probability(hmm, observations, path), best, 1e-6);
}

TEST(5, "an empty board scores nothing") {
    const auto plan = best_ship_plan({}, {});
    CHECK_EQ(plan.score, 0);
    CHECK(plan.ships.empty());
}

TEST(5, "mismatched rows are rejected") {
    const std::vector<int> top{1};
    const std::vector<int> bottom{1, 2};
    CHECK_THROWS_AS(best_ship_plan(top, bottom), std::invalid_argument);
}

TEST(5, "negative cells are left alone") {
    const std::vector<int> top{-5, -5};
    const std::vector<int> bottom{-1, -1};
    const auto plan = best_ship_plan(top, bottom);
    CHECK_EQ(plan.score, 0);
    CHECK(plan.ships.empty());
}

TEST(5, "a vertical ship can bridge a good and a bad cell") {
    const std::vector<int> top{9};
    const std::vector<int> bottom{-2};
    const auto plan = best_ship_plan(top, bottom);
    CHECK_EQ(plan.score, 7);
    REQUIRE_EQ(plan.ships.size(), 1);
    CHECK(plan.ships[0].vertical);
}

TEST(5, "two horizontal ships can overlap in columns on different rows") {
    const std::vector<int> top{4, 4};
    const std::vector<int> bottom{5, 5};
    const auto plan = best_ship_plan(top, bottom);
    CHECK_EQ(plan.score, 18);
    CHECK_EQ(plan.ships.size(), 2);
}

TEST(5, "random boards match trying every placement") {
    Rng rng;
    for (int trial = 0; trial < 1200; ++trial) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 7));
        const auto top = random_row(rng, n);
        const auto bottom = random_row(rng, n);
        INFO("top ", top, " bottom ", bottom);
        const auto plan = best_ship_plan(top, bottom);
        REQUIRE_EQ(score_ships(top, bottom, plan.ships), plan.score);
        REQUIRE_EQ(plan.score, brute_ships(top, bottom, 0, 0, 0));
    }
}

PERF(5, "a board of a million columns", 800) {
    Rng rng;
    const auto top = rng.ints(1'000'000, -50, 50);
    const auto bottom = rng.ints(1'000'000, -50, 50);
    CHECK_GT(best_ship_plan(top, bottom).score, 0);
}
