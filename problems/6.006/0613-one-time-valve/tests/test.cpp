#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

constexpr std::int64_t kNone = std::numeric_limits<std::int64_t>::max();

std::vector<std::int64_t> heads_after(const PipeNetwork& network) {
    std::vector<std::int64_t> after = network.head;
    for (const HeadDrop& drop : network.drops) {
        after[idx(drop.junction)] = drop.new_head;
    }
    return after;
}

// Plain recursion over (junction, fired): heads strictly drop in each phase, so this terminates.
void brute_walk(const PipeNetwork& network, const std::vector<std::int64_t>& after, int at, bool fired,
                int sink, std::int64_t length, std::int64_t& best) {
    if (at == sink) {
        best = std::min(best, length);
    }
    if (length >= best) {
        return;
    }
    if (!fired && at == network.valve) {
        brute_walk(network, after, at, true, sink, length, best);
    }
    const std::vector<std::int64_t>& heads = fired ? after : network.head;
    for (const Pipe& pipe : network.pipes) {
        if (pipe.a == at && heads[idx(at)] > heads[idx(pipe.b)]) {
            brute_walk(network, after, pipe.b, fired, sink, length + pipe.length, best);
        } else if (pipe.b == at && heads[idx(at)] > heads[idx(pipe.a)]) {
            brute_walk(network, after, pipe.a, fired, sink, length + pipe.length, best);
        }
    }
}

std::optional<std::int64_t> brute_shortest(const PipeNetwork& network, int source, int sink) {
    const auto after = heads_after(network);
    std::int64_t best = kNone;
    brute_walk(network, after, source, false, sink, 0, best);
    if (best == kNone) {
        return std::nullopt;
    }
    return best;
}

PipeNetwork random_network(Rng& rng, int n, int m) {
    PipeNetwork network;
    network.head.resize(idx(n));
    for (auto& h : network.head) {
        h = rng.uniform(0, 12);
    }
    for (int i = 0; i < m && n >= 2; ++i) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = static_cast<int>(rng.uniform(0, n - 1));
        if (a != b) {
            network.pipes.push_back({a, b, rng.uniform(1, 9)});
        }
    }
    network.valve = static_cast<int>(rng.uniform(0, n - 1));
    return network;
}

struct Instance {
    PipeNetwork network;
    int source;
    int sink;
};

// Random instances where the valve is usually on the way: the source sits high up and the valve is
// chosen among the junctions the robot can actually reach before firing.
Instance random_valve_instance(Rng& rng) {
    const int n = static_cast<int>(rng.uniform(2, 8));
    Instance instance{random_network(rng, n, static_cast<int>(rng.uniform(n, 3 * n))), 0, 0};
    PipeNetwork& network = instance.network;
    instance.source = rng.coin(0.85)
                          ? static_cast<int>(std::max_element(network.head.begin(), network.head.end()) -
                                             network.head.begin())
                          : static_cast<int>(rng.uniform(0, n - 1));
    std::vector<char> reachable(idx(n), 0);
    std::vector<int> stack{instance.source};
    reachable[idx(instance.source)] = 1;
    while (!stack.empty()) {
        const int at = stack.back();
        stack.pop_back();
        for (const Pipe& pipe : network.pipes) {
            const int to = pipe.a == at ? pipe.b : (pipe.b == at ? pipe.a : -1);
            if (to != -1 && network.head[idx(at)] > network.head[idx(to)] && reachable[idx(to)] == 0) {
                reachable[idx(to)] = 1;
                stack.push_back(to);
            }
        }
    }
    std::vector<int> candidates;
    for (int v = 0; v < n; ++v) {
        if (reachable[idx(v)] != 0 && v != instance.source) {
            candidates.push_back(v);
        }
    }
    network.valve = candidates.empty()
                        ? instance.source
                        : candidates[idx(static_cast<int>(rng.uniform(0, static_cast<std::int64_t>(candidates.size()) - 1)))];
    for (int j = 0; j < n; ++j) {
        if (j != network.valve && rng.coin(0.6)) {
            network.drops.push_back({j, network.head[idx(j)] - rng.uniform(1, 12)});
        }
    }
    instance.sink = static_cast<int>(rng.uniform(0, n - 1));
    return instance;
}

void check_plan(const PipeNetwork& network, int source, int sink, const DriftPlan& plan,
                std::int64_t expected) {
    REQUIRE(!plan.route.empty());
    CHECK_EQ(plan.route.front(), source);
    CHECK_EQ(plan.route.back(), sink);
    CHECK_EQ(plan.length, expected);
    if (plan.trigger) {
        REQUIRE(*plan.trigger < plan.route.size());
        REQUIRE_EQ(plan.route[*plan.trigger], network.valve);
    }
    std::vector<std::int64_t> heads = network.head;
    std::int64_t walked = 0;
    for (std::size_t i = 0; i < plan.route.size(); ++i) {
        if (plan.trigger && *plan.trigger == i) {
            heads = heads_after(network);
        }
        if (i + 1 == plan.route.size()) {
            break;
        }
        const int from = plan.route[i];
        const int to = plan.route[i + 1];
        REQUIRE(to >= 0 && to < static_cast<int>(network.head.size()));
        REQUIRE(heads[idx(from)] > heads[idx(to)]);
        std::int64_t shortest = kNone;
        for (const Pipe& pipe : network.pipes) {
            if ((pipe.a == from && pipe.b == to) || (pipe.a == to && pipe.b == from)) {
                shortest = std::min(shortest, pipe.length);
            }
        }
        REQUIRE(shortest != kNone);
        walked += shortest;
    }
    CHECK_EQ(walked, expected);
}

}  // namespace

TEST(1, "start equals end costs nothing") {
    const PipeNetwork network{{5}, {}, {}, 0};
    CHECK_EQ(shortest_drift(network, 0, 0), std::optional<std::int64_t>(0));
}

TEST(1, "water only flows downhill") {
    const PipeNetwork network{{9, 4}, {{0, 1, 3}}, {}, 1};
    CHECK_EQ(shortest_drift(network, 0, 1), std::optional<std::int64_t>(3));
    CHECK_EQ(shortest_drift(network, 1, 0), std::optional<std::int64_t>());
}

TEST(1, "equal heads block both directions") {
    const PipeNetwork network{{5, 5}, {{0, 1, 1}}, {}, 0};
    CHECK_EQ(shortest_drift(network, 0, 1), std::optional<std::int64_t>());
    CHECK_EQ(shortest_drift(network, 1, 0), std::optional<std::int64_t>());
}

TEST(1, "the shortest downhill chain is chosen") {
    const PipeNetwork network{{10, 7, 3}, {{0, 1, 2}, {1, 2, 2}, {0, 2, 9}}, {}, 1};
    CHECK_EQ(shortest_drift(network, 0, 2), std::optional<std::int64_t>(4));
}

TEST(1, "bad junctions throw out_of_range") {
    const PipeNetwork network{{1, 2}, {}, {}, 0};
    CHECK_THROWS_AS(shortest_drift(network, 2, 0), std::out_of_range);
    CHECK_THROWS_AS(shortest_drift(network, 0, -1), std::out_of_range);
}

TEST(1, "random networks without drops match exhaustive search") {
    Rng rng;
    int reached = 0;
    for (int round = 0; round < 1500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 8));
        const PipeNetwork network = random_network(rng, n, static_cast<int>(rng.uniform(0, 2 * n)));
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        const int t = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " s=", s, " t=", t);
        const auto expected = brute_shortest(network, s, t);
        REQUIRE_EQ(shortest_drift(network, s, t), expected);
        reached += expected.has_value() ? 1 : 0;
    }
    CHECK_GT(reached, 300);
}

PipeNetwork big_chain_network(Rng& rng, int n, int extra, bool with_drops) {
    PipeNetwork network;
    network.head.resize(idx(n));
    for (int v = 0; v < n; ++v) {
        network.head[idx(v)] = 3 * (static_cast<std::int64_t>(n) - v) * 1'000;
    }
    for (int v = 0; v + 1 < n; ++v) {
        network.pipes.push_back({v, v + 1, rng.uniform(1, 50)});
    }
    for (int i = 0; i < extra; ++i) {
        network.pipes.push_back({static_cast<int>(rng.uniform(0, n - 1)),
                                 static_cast<int>(rng.uniform(0, n - 1)), rng.uniform(1, 50)});
    }
    network.valve = n / 2;
    if (with_drops) {
        for (int v = n / 2 + 1; v < n; v += 2) {
            network.drops.push_back({v, network.head[idx(v)] - 1'000});
        }
    }
    rng.shuffle(network.pipes);
    return network;
}

// A round-based relaxation pass needs on the order of 500,000 rounds on these chains.
PERF(1, "a very long chain in shuffled order is linear", 2000) {
    Rng rng;
    const int n = 500'000;
    const PipeNetwork network = big_chain_network(rng, n, 500'000, false);
    const auto length = shortest_drift(network, 0, n - 1);
    CHECK(length.has_value());
}

TEST(2, "firing the valve can open a new downhill path") {
    // Junction 2 starts above 1, so 1 -> 2 is uphill; after the drop it runs downhill.
    const PipeNetwork network{{10, 5, 7}, {{0, 1, 4}, {1, 2, 1}, {0, 2, 9}}, {{2, 3}}, 1};
    CHECK_EQ(shortest_drift(network, 0, 2), std::optional<std::int64_t>(5));
}

TEST(2, "firing only happens at the valve junction") {
    // Dropping junction 1 would not open 1 -> 2 anyway, and the valve at 2 comes too late to help.
    const PipeNetwork network{{10, 5, 7}, {{0, 1, 4}, {1, 2, 1}, {0, 2, 9}}, {{1, 2}}, 2};
    CHECK_EQ(shortest_drift(network, 0, 2), std::optional<std::int64_t>(9));
}

TEST(2, "firing is optional when it would hurt") {
    // Firing at the start would drop 1 below 2 and strand the robot, so it must not fire.
    const PipeNetwork network{{9, 5, 2}, {{0, 1, 1}, {1, 2, 1}}, {{1, 1}}, 0};
    CHECK_EQ(shortest_drift(network, 0, 2), std::optional<std::int64_t>(2));
}

TEST(2, "a route may need the old heads first, then the new heads") {
    // 2 -> 1 only works before the drop, and 1 -> 3 only works after it.
    const PipeNetwork network{{10, 8, 9, 9}, {{0, 2, 1}, {2, 1, 1}, {1, 3, 1}}, {{2, 7}, {3, 5}}, 1};
    CHECK_EQ(shortest_drift(network, 0, 3), std::optional<std::int64_t>(3));
}

TEST(2, "after firing the old direction is gone") {
    // Firing at the source would turn 2 -> 1 uphill, so the only working route leaves the valve alone.
    const PipeNetwork network{{9, 4, 5, 1}, {{0, 2, 1}, {2, 1, 1}, {1, 3, 1}}, {{2, 3}}, 0};
    CHECK_EQ(shortest_drift(network, 0, 3), std::optional<std::int64_t>(3));
}

TEST(2, "the valve can be useless when it lies off the route") {
    const PipeNetwork network{{9, 6, 7, 2}, {{0, 1, 3}, {0, 2, 1}, {2, 1, 1}, {1, 3, 1}}, {{1, 4}}, 2};
    CHECK_EQ(shortest_drift(network, 0, 1), std::optional<std::int64_t>(2));
}

TEST(2, "random networks with drops match exhaustive search") {
    Rng rng;
    int differs = 0;
    for (int round = 0; round < 4000; ++round) {
        const Instance instance = random_valve_instance(rng);
        const PipeNetwork& network = instance.network;
        const int s = instance.source;
        const int t = instance.sink;
        INFO("round ", round, " s=", s, " t=", t, " valve=", network.valve);
        const auto expected = brute_shortest(network, s, t);
        REQUIRE_EQ(shortest_drift(network, s, t), expected);
        PipeNetwork without = network;
        without.drops.clear();
        differs += brute_shortest(without, s, t) != expected ? 1 : 0;
    }
    CHECK_GT(differs, 200);
}

PERF(2, "the valve does not change the complexity class", 2500) {
    Rng rng;
    const int n = 500'000;
    const PipeNetwork network = big_chain_network(rng, n, 500'000, true);
    const auto length = shortest_drift(network, 0, n - 1);
    CHECK(length.has_value());
}

TEST(3, "no route gives nullopt") {
    const PipeNetwork network{{1, 2}, {}, {}, 0};
    CHECK(!plan_drift(network, 0, 1).has_value());
}

TEST(3, "start equals end is a single-element route") {
    const PipeNetwork network{{5}, {}, {}, 0};
    const auto plan = plan_drift(network, 0, 0);
    REQUIRE(plan.has_value());
    CHECK_EQ(plan->route, std::vector<int>{0});
    CHECK_EQ(plan->length, 0);
}

TEST(3, "a plan without firing has no trigger") {
    const PipeNetwork network{{10, 7, 3}, {{0, 1, 2}, {1, 2, 2}}, {}, 2};
    const auto plan = plan_drift(network, 0, 2);
    REQUIRE(plan.has_value());
    check_plan(network, 0, 2, *plan, 4);
    CHECK(!plan->trigger.has_value());
}

TEST(3, "a plan that needs the valve reports where it fires") {
    const PipeNetwork network{{10, 5, 7}, {{0, 1, 4}, {1, 2, 1}, {0, 2, 9}}, {{2, 3}}, 1};
    const auto plan = plan_drift(network, 0, 2);
    REQUIRE(plan.has_value());
    check_plan(network, 0, 2, *plan, 5);
    REQUIRE(plan->trigger.has_value());
    CHECK_EQ(plan->route, std::vector<int>{0, 1, 2});
    CHECK_EQ(*plan->trigger, 1);
}

TEST(3, "random networks produce legal shortest plans") {
    Rng rng;
    int fired = 0;
    for (int round = 0; round < 3000; ++round) {
        const Instance instance = random_valve_instance(rng);
        const PipeNetwork& network = instance.network;
        const int s = instance.source;
        const int t = instance.sink;
        INFO("round ", round, " s=", s, " t=", t, " valve=", network.valve);
        const auto expected = brute_shortest(network, s, t);
        const auto plan = plan_drift(network, s, t);
        REQUIRE_EQ(plan.has_value(), expected.has_value());
        if (plan) {
            check_plan(network, s, t, *plan, *expected);
            fired += plan->trigger.has_value() ? 1 : 0;
        }
    }
    CHECK_GT(fired, 100);
}

PERF(3, "plan extraction stays linear", 2500) {
    Rng rng;
    const int n = 500'000;
    const PipeNetwork network = big_chain_network(rng, n, 500'000, true);
    const auto plan = plan_drift(network, 0, n - 1);
    REQUIRE(plan.has_value());
    CHECK_EQ(plan->route.front(), 0);
}
