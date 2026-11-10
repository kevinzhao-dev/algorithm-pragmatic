#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

// Any state's total change equals potential differences, so every cycle sums to zero.
std::vector<Transition> random_conservative(Rng& rng, int n, int m) {
    std::vector<std::int64_t> potential(idx(n));
    for (auto& h : potential) {
        h = rng.uniform(-50, 50);
    }
    std::vector<Transition> transitions;
    for (int i = 0; i < m; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        const int v = static_cast<int>(rng.uniform(0, n - 1));
        transitions.push_back({u, v, potential[idx(v)] - potential[idx(u)]});
    }
    return transitions;
}

// Walks every simple cycle, trying each parallel transition separately.
void scan_cycles(int start, int at, std::int64_t total, const std::vector<std::vector<std::size_t>>& out,
                 const std::vector<Transition>& transitions, std::vector<char>& on_path, bool& unbalanced) {
    for (const std::size_t i : out[idx(at)]) {
        const Transition& t = transitions[i];
        if (t.to == start) {
            unbalanced = unbalanced || total + t.delta != 0;
        } else if (t.to > start && on_path[idx(t.to)] == 0) {
            on_path[idx(t.to)] = 1;
            scan_cycles(start, t.to, total + t.delta, out, transitions, on_path, unbalanced);
            on_path[idx(t.to)] = 0;
        }
    }
}

bool brute_conservative(int n, const std::vector<Transition>& transitions) {
    std::vector<std::vector<std::size_t>> out(idx(n));
    for (std::size_t i = 0; i < transitions.size(); ++i) {
        out[idx(transitions[i].from)].push_back(i);
    }
    bool unbalanced = false;
    for (int start = 0; start < n && !unbalanced; ++start) {
        std::vector<char> on_path(idx(n), 0);
        on_path[idx(start)] = 1;
        scan_cycles(start, start, 0, out, transitions, on_path, unbalanced);
    }
    return !unbalanced;
}

void check_cycle(int n, const std::vector<Transition>& transitions, const std::vector<std::size_t>& cycle) {
    REQUIRE(!cycle.empty());
    std::set<int> seen;
    std::int64_t total = 0;
    for (std::size_t i = 0; i < cycle.size(); ++i) {
        REQUIRE(cycle[i] < transitions.size());
        const Transition& current = transitions[cycle[i]];
        const Transition& next = transitions[cycle[(i + 1) % cycle.size()]];
        REQUIRE_EQ(current.to, next.from);
        REQUIRE(current.from >= 0 && current.from < n);
        CHECK(seen.insert(current.from).second);
        total += current.delta;
    }
    CHECK_NE(total, 0);
}

}  // namespace

TEST(1, "no transitions is trivially conservative") {
    CHECK(is_conservative(1, {}));
    CHECK(is_conservative(3, {}));
}

TEST(1, "a chain without a cycle is conservative whatever the deltas") {
    const std::vector<Transition> transitions = {{0, 1, 5}, {1, 2, -9}, {3, 2, 100}};
    CHECK(is_conservative(4, transitions));
}

TEST(1, "a balanced loop is conservative") {
    const std::vector<Transition> transitions = {{0, 1, 5}, {1, 2, -2}, {2, 0, -3}};
    CHECK(is_conservative(3, transitions));
}

TEST(1, "a loop that gains points is not conservative") {
    const std::vector<Transition> transitions = {{0, 1, 5}, {1, 2, -2}, {2, 0, -2}};
    CHECK(!is_conservative(3, transitions));
}

TEST(1, "a loop that loses points is not conservative") {
    const std::vector<Transition> transitions = {{0, 1, -1}, {1, 0, 0}};
    CHECK(!is_conservative(2, transitions));
}

TEST(1, "a self-loop must be exactly zero") {
    const std::vector<Transition> zero = {{0, 0, 0}};
    CHECK(is_conservative(1, zero));
    const std::vector<Transition> gain = {{0, 0, 1}};
    CHECK(!is_conservative(1, gain));
}

TEST(1, "parallel transitions can disagree only if a cycle exposes it") {
    const std::vector<Transition> no_cycle = {{0, 1, 5}, {0, 1, 9}};
    CHECK(is_conservative(2, no_cycle));
    const std::vector<Transition> exposed = {{0, 1, 5}, {0, 1, 9}, {1, 0, -5}};
    CHECK(!is_conservative(2, exposed));
}

TEST(1, "a bad loop in a far component still counts") {
    const std::vector<Transition> transitions = {{0, 1, 3}, {2, 3, 1}, {3, 2, 1}};
    CHECK(!is_conservative(4, transitions));
}

TEST(1, "random potentials give conservative systems") {
    Rng rng;
    for (int round = 0; round < 800; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 40));
        const auto transitions = random_conservative(rng, n, static_cast<int>(rng.uniform(0, 4 * n)));
        INFO("round ", round);
        CHECK(is_conservative(n, transitions));
    }
}

TEST(1, "random systems match enumeration of simple cycles") {
    Rng rng;
    int conservative = 0;
    int broken = 0;
    for (int round = 0; round < 2500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 8));
        const int m = static_cast<int>(rng.uniform(0, 2 * n));
        std::vector<Transition> transitions = random_conservative(rng, n, m);
        if (!transitions.empty() && rng.coin(0.6)) {
            transitions[idx(static_cast<int>(rng.uniform(0, m - 1)))].delta += rng.coin() ? 1 : -1;
        }
        INFO("round ", round, " n=", n);
        const bool expected = brute_conservative(n, transitions);
        REQUIRE_EQ(is_conservative(n, transitions), expected);
        (expected ? conservative : broken) += 1;
    }
    CHECK_GT(conservative, 200);
    CHECK_GT(broken, 200);
}

TEST(3, "a ring of half a million states does not overflow the stack") {
    const int n = 500'000;
    std::vector<Transition> transitions;
    transitions.reserve(idx(n));
    for (int v = 0; v < n; ++v) {
        transitions.push_back({v, (v + 1) % n, v % 2 == 0 ? 1 : -1});
    }
    CHECK(is_conservative(n, transitions));
    transitions.back().delta += 2;
    CHECK(!is_conservative(n, transitions));
}

// All-pairs (Floyd-Warshall) needs 2.7e10 steps here; the budget allows a couple of Bellman-Ford runs.
PERF(1, "a few thousand states fit in the quadratic budget", 2500) {
    Rng rng;
    const int n = 3000;
    std::vector<Transition> transitions;
    for (int v = n - 1; v > 0; --v) {
        transitions.push_back({v - 1, v, -1});
    }
    for (int i = 0; i < 2 * n; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        transitions.push_back({u, u, 0});
    }
    transitions.push_back({n - 1, 0, n - 1});
    CHECK(is_conservative(n, transitions));
    transitions.back().delta += 1;
    CHECK(!is_conservative(n, transitions));
}

TEST(2, "a conservative system has no bad cycle") {
    const std::vector<Transition> transitions = {{0, 1, 5}, {1, 2, -2}, {2, 0, -3}, {3, 0, 11}};
    CHECK_EQ(find_unbalanced_cycle(4, transitions), std::optional<std::vector<std::size_t>>());
    CHECK_EQ(find_unbalanced_cycle(3, {}), std::optional<std::vector<std::size_t>>());
}

TEST(2, "a positive and a negative loop are both found") {
    const std::vector<Transition> positive = {{0, 1, 5}, {1, 0, -4}};
    const auto first = find_unbalanced_cycle(2, positive);
    REQUIRE(first.has_value());
    check_cycle(2, positive, *first);

    const std::vector<Transition> negative = {{0, 1, -5}, {1, 0, 4}};
    const auto second = find_unbalanced_cycle(2, negative);
    REQUIRE(second.has_value());
    check_cycle(2, negative, *second);
}

TEST(2, "a self-loop with a non-zero delta is reported by its index") {
    const std::vector<Transition> transitions = {{0, 1, 1}, {1, 1, 3}};
    CHECK_EQ(find_unbalanced_cycle(2, transitions), std::optional(std::vector<std::size_t>{1}));
}

TEST(2, "indices identify which parallel transition was used") {
    const std::vector<Transition> transitions = {{0, 1, 5}, {0, 1, 7}, {1, 0, -5}};
    const auto cycle = find_unbalanced_cycle(2, transitions);
    REQUIRE(cycle.has_value());
    check_cycle(2, transitions, *cycle);
    CHECK(std::find(cycle->begin(), cycle->end(), 1) != cycle->end());
}

TEST(2, "the returned cycle excludes tails leading into it") {
    const std::vector<Transition> transitions = {{0, 1, 1}, {1, 2, 1}, {2, 3, 2}, {3, 2, 1}};
    const auto cycle = find_unbalanced_cycle(4, transitions);
    REQUIRE(cycle.has_value());
    check_cycle(4, transitions, *cycle);
    CHECK_EQ(cycle->size(), 2);
}

TEST(2, "random systems produce valid witnesses") {
    Rng rng;
    int reported = 0;
    for (int round = 0; round < 2500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 8));
        const int m = static_cast<int>(rng.uniform(0, 2 * n));
        std::vector<Transition> transitions = random_conservative(rng, n, m);
        if (!transitions.empty() && rng.coin(0.6)) {
            transitions[idx(static_cast<int>(rng.uniform(0, m - 1)))].delta += rng.uniform(-3, 3);
        }
        INFO("round ", round, " n=", n);
        const auto cycle = find_unbalanced_cycle(n, transitions);
        const bool expected = brute_conservative(n, transitions);
        REQUIRE_EQ(cycle.has_value(), !expected);
        if (cycle) {
            check_cycle(n, transitions, *cycle);
            ++reported;
        }
    }
    CHECK_GT(reported, 200);
}

TEST(3, "large mixed systems agree with a direct potential construction") {
    Rng rng;
    for (int round = 0; round < 40; ++round) {
        const int n = 200;
        std::vector<Transition> transitions = random_conservative(rng, n, 800);
        CHECK(is_conservative(n, transitions));
        // Flip one transition that certainly lies on a cycle.
        transitions.push_back({transitions[0].to, transitions[0].from, -transitions[0].delta + 1});
        INFO("round ", round);
        CHECK(!is_conservative(n, transitions));
    }
}

// Edges are listed back to front along the chain, so sweeping edges takes about V rounds.
PERF(3, "millions of transitions in linear time", 3000) {
    Rng rng;
    const int n = 2'000'000;
    std::vector<Transition> transitions;
    transitions.reserve(idx(n) + 500'000);
    for (int v = n - 1; v > 0; --v) {
        transitions.push_back({v - 1, v, -3});
    }
    for (int i = 0; i < 500'000; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        const int v = static_cast<int>(rng.uniform(0, n - 1));
        transitions.push_back({std::max(u, v), std::min(u, v), 3 * (std::max(u, v) - std::min(u, v))});
    }
    CHECK(is_conservative(n, transitions));
    transitions.back().delta += 1;
    CHECK(!is_conservative(n, transitions));
}
