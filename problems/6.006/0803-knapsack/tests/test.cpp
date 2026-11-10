#include <cstdint>
#include <vector>

#include "07_heap_meter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Items = std::vector<Item>;

std::int64_t exhaustive(const Items& items, std::int64_t capacity) {
    std::int64_t best = 0;
    for (std::size_t mask = 0; mask < (std::size_t{1} << items.size()); ++mask) {
        std::int64_t size = 0;
        std::int64_t value = 0;
        for (std::size_t i = 0; i < items.size(); ++i) {
            if ((mask >> i) & 1) {
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

Items random_items(Rng& rng, std::size_t max_count, std::int64_t max_size, std::int64_t max_value) {
    Items items(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(max_count))));
    for (Item& item : items) {
        item = {rng.uniform(1, max_size), rng.uniform(0, max_value)};
    }
    return items;
}

}  // namespace

TEST(1, "no items are worth nothing") {
    CHECK_EQ(knapsack_value({}, 100), 0);
}

TEST(1, "zero capacity holds nothing") {
    const Items items{{1, 10}};
    CHECK_EQ(knapsack_value(items, 0), 0);
}

TEST(1, "each item is used at most once") {
    const Items items{{1, 10}};
    CHECK_EQ(knapsack_value(items, 100), 10);
}

TEST(1, "greedy by value density is not optimal") {
    // Density favors the first item, but taking it blocks the other two.
    const Items items{{6, 7}, {5, 5}, {5, 5}};
    CHECK_EQ(knapsack_value(items, 10), 10);
}

TEST(1, "an item exactly filling the capacity fits") {
    const Items items{{7, 3}};
    CHECK_EQ(knapsack_value(items, 7), 3);
}

TEST(1, "random instances match enumeration") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Items items = random_items(rng, 12, 12, 20);
        const std::int64_t capacity = rng.uniform(0, 40);
        INFO("trial ", trial, " capacity ", capacity);
        REQUIRE_EQ(knapsack_value(items, capacity), exhaustive(items, capacity));
    }
}

PERF(1, "two thousand items and capacity two hundred thousand", 2000) {
    Rng rng;
    Items items(2000);
    for (Item& item : items) {
        item = {rng.uniform(1, 500), rng.uniform(1, 100)};
    }
    CHECK_GT(knapsack_value(items, 200'000), 0);
}

TEST(2, "no items chosen when nothing fits") {
    const Items items{{5, 10}};
    CHECK(knapsack_items(items, 3).empty());
}

TEST(2, "the chosen indices are ascending and honest") {
    const Items items{{3, 4}, {4, 5}, {2, 3}};
    const auto chosen = knapsack_items(items, 5);
    std::int64_t size = 0;
    std::int64_t value = 0;
    for (std::size_t k = 0; k < chosen.size(); ++k) {
        REQUIRE(chosen[k] < items.size());
        REQUIRE(k == 0 || chosen[k - 1] < chosen[k]);
        size += items[chosen[k]].size;
        value += items[chosen[k]].value;
    }
    CHECK_LE(size, 5);
    CHECK_EQ(value, knapsack_value(items, 5));
}

TEST(2, "random selections are valid and optimal") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Items items = random_items(rng, 12, 12, 20);
        const std::int64_t capacity = rng.uniform(0, 40);
        INFO("trial ", trial, " capacity ", capacity);
        const auto chosen = knapsack_items(items, capacity);
        std::int64_t size = 0;
        std::int64_t value = 0;
        for (std::size_t k = 0; k < chosen.size(); ++k) {
            REQUIRE(chosen[k] < items.size());
            REQUIRE(k == 0 || chosen[k - 1] < chosen[k]);
            size += items[chosen[k]].size;
            value += items[chosen[k]].value;
        }
        REQUIRE(size <= capacity);
        REQUIRE_EQ(value, exhaustive(items, capacity));
    }
}

TEST(3, "knapsack_value keeps memory linear in the capacity") {
    Rng rng;
    Items items(500);
    for (Item& item : items) {
        item = {rng.uniform(1, 300), rng.uniform(1, 100)};
    }
    const std::int64_t capacity = 50'000;
    heap_meter::Scope scope;
    const std::int64_t value = knapsack_value(items, capacity);
    CHECK_LE(scope.peak_bytes(), 24 * static_cast<std::size_t>(capacity));
    CHECK_GT(value, 0);
}

TEST(3, "the rolling array still treats each item as single-use") {
    const Items items{{2, 3}};
    CHECK_EQ(knapsack_value(items, 8), 3);
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const Items random = random_items(rng, 12, 8, 20);
        const std::int64_t capacity = rng.uniform(0, 30);
        INFO("trial ", trial, " capacity ", capacity);
        REQUIRE_EQ(knapsack_value(random, capacity), exhaustive(random, capacity));
    }
}

TEST(4, "no items are worth nothing regardless of capacity") {
    CHECK_EQ(knapsack_by_value({}, 1'000'000'000'000'000), 0);
}

TEST(4, "huge capacities need no huge table") {
    const Items items{{400'000'000'000'000, 9}, {500'000'000'000'000, 7}, {700'000'000'000'000, 12}};
    CHECK_EQ(knapsack_by_value(items, 1'000'000'000'000'000), 16);
}

TEST(4, "agrees with capacity-based DP on small instances") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Items items = random_items(rng, 12, 10, 15);
        const std::int64_t capacity = rng.uniform(0, 40);
        INFO("trial ", trial, " capacity ", capacity);
        REQUIRE_EQ(knapsack_by_value(items, capacity), knapsack_value(items, capacity));
        REQUIRE_EQ(knapsack_by_value(items, capacity), exhaustive(items, capacity));
    }
}

TEST(4, "items that do not fit are never counted") {
    const Items items{{100, 1}};
    CHECK_EQ(knapsack_by_value(items, 99), 0);
}

PERF(4, "capacity in the quadrillions with small values", 1500) {
    Rng rng;
    Items items(1500);
    for (Item& item : items) {
        item = {rng.uniform(1, 2'000'000'000'000), rng.uniform(1, 100)};
    }
    CHECK_GT(knapsack_by_value(items, 1'000'000'000'000'000), 0);
}
