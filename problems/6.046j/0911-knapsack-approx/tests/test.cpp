#include <algorithm>
#include <numeric>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

long long brute_opt(const std::vector<Item>& items, long long capacity) {
    const int n = static_cast<int>(items.size());
    long long best = 0;
    for (int mask = 0; mask < (1 << n); ++mask) {
        long long size = 0, value = 0;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) {
                size += items[i].size;
                value += items[i].value;
            }
        }
        if (size <= capacity) {
            best = std::max(best, value);
        }
    }
    return best;
}

void check_selection(const std::vector<Item>& items, long long capacity, const Selection& s) {
    long long size = 0, value = 0;
    REQUIRE(std::is_sorted(s.items.begin(), s.items.end()));
    REQUIRE(std::adjacent_find(s.items.begin(), s.items.end()) == s.items.end());
    for (int i : s.items) {
        REQUIRE(i >= 0 && i < static_cast<int>(items.size()));
        size += items[i].size;
        value += items[i].value;
    }
    CHECK_EQ(size, s.size);
    CHECK_EQ(value, s.value);
    CHECK_LE(size, capacity);
}

std::vector<Item> random_items(Rng& rng, int n, long long max_size, long long max_value) {
    std::vector<Item> items(n);
    for (auto& item : items) {
        item = {rng.uniform(1, max_size), rng.uniform(1, max_value)};
    }
    return items;
}

}  // namespace

TEST(1, "greedy takes items in density order") {
    const std::vector<Item> items{{5, 5}, {1, 4}, {4, 4}};
    const auto s = greedy_by_density(items, 6);
    check_selection(items, 6, s);
    CHECK_EQ(s.items, (std::vector<int>{0, 1}));
    CHECK_EQ(s.value, 9);
}

TEST(1, "density ties prefer the earlier index") {
    const std::vector<Item> items{{2, 4}, {1, 2}, {3, 6}};
    const auto s = greedy_by_density(items, 4);
    check_selection(items, 4, s);
    CHECK_EQ(s.items, (std::vector<int>{0, 1}));
}

TEST(1, "an item that does not fit is skipped") {
    const std::vector<Item> items{{3, 9}, {4, 8}, {1, 1}};
    const auto s = greedy_by_density(items, 4);
    CHECK_EQ(s.items, (std::vector<int>{0, 2}));
}

TEST(1, "the trap fools density greedy by a factor of k") {
    for (long long k : {1LL, 2LL, 5LL, 100LL, 1'000'000LL}) {
        const DensityTrap trap = density_trap(k);
        REQUIRE(!trap.items.empty());
        for (const Item& item : trap.items) {
            REQUIRE(item.size >= 1 && item.value >= 1 && item.size <= trap.capacity);
        }
        const auto greedy = greedy_by_density(trap.items, trap.capacity);
        check_selection(trap.items, trap.capacity, greedy);
        INFO("k=", k);
        REQUIRE(trap.items.size() <= 20);
        CHECK_LE(__int128(greedy.value) * k, __int128(brute_opt(trap.items, trap.capacity)));
    }
}

TEST(2, "everything fits so everything is taken") {
    const std::vector<Item> items{{1, 2}, {3, 1}};
    const auto s = greedy_two_approx(items, 10);
    CHECK_EQ(s.items, (std::vector<int>{0, 1}));
}

TEST(2, "the overflow item can be the better answer") {
    // Density order: index 0 first, then index 1 overflows and is worth more than the prefix.
    const std::vector<Item> items{{1, 3}, {100, 200}};
    const auto s = greedy_two_approx(items, 100);
    check_selection(items, 100, s);
    CHECK_EQ(s.items, std::vector<int>{1});
    CHECK_EQ(s.value, 200);
}

TEST(2, "otherwise the density prefix is returned") {
    const std::vector<Item> items{{2, 10}, {3, 9}, {4, 2}};
    const auto s = greedy_two_approx(items, 5);
    CHECK_EQ(s.items, (std::vector<int>{0, 1}));
}

TEST(2, "random instances: valid and within a factor of two") {
    Rng rng;
    for (int round = 0; round < 2000; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 14));
        const long long capacity = rng.uniform(1, 30);
        auto items = random_items(rng, n, capacity, 40);
        const auto s = greedy_two_approx(items, capacity);
        INFO("round ", round);
        check_selection(items, capacity, s);
        CHECK_GE(2 * s.value, brute_opt(items, capacity));
    }
}

PERF(2, "two approximation on a large catalog", 2500) {
    Rng rng;
    auto items = random_items(rng, 1'000'000, 1000, 1'000'000);
    const auto s = greedy_two_approx(items, 100'000'000);
    CHECK_GT(s.value, 0);
}

TEST(3, "no items") {
    const auto s = exact_by_value(std::vector<Item>{}, 5);
    CHECK_EQ(s.value, 0);
    CHECK(s.items.empty());
}

TEST(3, "items too large are ignored") {
    const std::vector<Item> items{{7, 100}, {3, 4}};
    const auto s = exact_by_value(items, 5);
    CHECK_EQ(s.items, std::vector<int>{1});
}

TEST(3, "picks the combination, not the single best item") {
    const std::vector<Item> items{{4, 7}, {3, 4}, {3, 4}};
    const auto s = exact_by_value(items, 6);
    check_selection(items, 6, s);
    CHECK_EQ(s.value, 8);
}

TEST(3, "random instances match brute force") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 14));
        const long long capacity = rng.uniform(1, 40);
        auto items = random_items(rng, n, 20, 30);
        INFO("round ", round);
        const auto s = exact_by_value(items, capacity);
        check_selection(items, capacity, s);
        CHECK_EQ(s.value, brute_opt(items, capacity));
    }
}

PERF(3, "pseudo-polynomial DP with small values", 500) {
    Rng rng;
    auto items = random_items(rng, 400, 1'000'000, 400);
    const auto s = exact_by_value(items, 100'000'000);
    CHECK_GT(s.value, 0);
}

TEST(4, "a single item fits") {
    const std::vector<Item> items{{2, 5}};
    const auto s = knapsack_fptas(items, 3, 0.5);
    CHECK_EQ(s.value, 5);
}

TEST(4, "guarantees (1-eps) of the optimum on brute-force sized inputs") {
    Rng rng;
    for (int round = 0; round < 1500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 14));
        const long long capacity = rng.uniform(1, 40);
        auto items = random_items(rng, n, capacity, 1000);
        const double eps = rng.real() * 0.8 + 0.05;
        const auto s = knapsack_fptas(items, capacity, eps);
        INFO("round ", round, " eps=", eps);
        check_selection(items, capacity, s);
        CHECK_GE(static_cast<double>(s.value),
                 (1.0 - eps) * static_cast<double>(brute_opt(items, capacity)) - 1e-6);
    }
}

TEST(4, "scaling must not round small items out of the picture") {
    // One big item against 99 small ones that only pay off together: too coarse a scale rounds
    // the small values away and the guarantee fails.
    for (const auto& [eps, small_value] :
         std::vector<std::pair<double, long long>>{{0.5, 38}, {0.25, 20}, {0.1, 12}}) {
        std::vector<Item> items{{99, 1000}};
        items.resize(100, Item{1, small_value});
        const long long opt = 99 * small_value;
        const auto s = knapsack_fptas(items, 99, eps);
        INFO("eps=", eps);
        check_selection(items, 99, s);
        CHECK_GE(static_cast<double>(s.value), (1.0 - eps) * static_cast<double>(opt) - 1e-6);
    }
}

TEST(4, "accuracy improves as eps shrinks, compared with exact DP") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        const int n = static_cast<int>(rng.uniform(20, 60));
        auto items = random_items(rng, n, 1000, 1000);
        const long long capacity = rng.uniform(5000, 20'000);
        const long long opt = exact_by_value(items, capacity).value;
        for (double eps : {0.5, 0.2, 0.05}) {
            const auto s = knapsack_fptas(items, capacity, eps);
            INFO("round ", round, " eps=", eps);
            check_selection(items, capacity, s);
            CHECK_GE(static_cast<double>(s.value), (1.0 - eps) * static_cast<double>(opt) - 1e-6);
            CHECK_LE(s.value, opt);
        }
    }
}

PERF(4, "huge values, runtime depends only on n and 1/eps", 1000) {
    Rng rng;
    auto items = random_items(rng, 300, 1'000'000, 1'000'000'000);
    const auto s = knapsack_fptas(items, 100'000'000, 0.2);
    CHECK_GT(s.value, 0);
}
