#include <cstdint>
#include <numeric>
#include <optional>
#include <set>
#include <vector>

#include "07_heap_meter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Values = std::vector<int>;

std::set<std::int64_t> all_sums(const Values& values) {
    std::set<std::int64_t> sums{0};
    for (const int value : values) {
        std::set<std::int64_t> added = sums;
        for (const std::int64_t sum : sums) {
            added.insert(sum + value);
        }
        sums = std::move(added);
    }
    return sums;
}

Values random_positive(Rng& rng, std::size_t max_count, int max_value) {
    return rng.ints(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(max_count))), 1,
                    max_value);
}

// The smallest achievable difference between the two sides, by enumerating every subset.
std::int64_t exhaustive_difference(const Values& values) {
    const std::int64_t total = std::accumulate(values.begin(), values.end(), std::int64_t{0});
    std::int64_t best = total;
    for (std::size_t mask = 0; mask < (std::size_t{1} << values.size()); ++mask) {
        std::int64_t side = 0;
        for (std::size_t i = 0; i < values.size(); ++i) {
            if ((mask >> i) & 1) {
                side += values[i];
            }
        }
        best = std::min(best, std::abs(total - 2 * side));
    }
    return best;
}

}  // namespace

TEST(1, "the empty subset reaches zero") {
    CHECK(subset_sum({}, 0));
    const Values values{4, 7};
    CHECK(subset_sum(values, 0));
}

TEST(1, "an element is usable only once") {
    const Values values{3};
    CHECK(subset_sum(values, 3));
    CHECK(!subset_sum(values, 6));
}

TEST(1, "unreachable targets are rejected") {
    const Values values{2, 4, 8};
    CHECK(!subset_sum(values, 7));
    CHECK(subset_sum(values, 14));
}

TEST(1, "random sets match enumeration") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Values values = random_positive(rng, 12, 15);
        const int target = static_cast<int>(rng.uniform(0, 100));
        INFO("values ", values, " target ", target);
        REQUIRE_EQ(subset_sum(values, target), all_sums(values).contains(target));
    }
}

PERF(1, "five hundred values and target one million", 1000) {
    Rng rng;
    const Values values = rng.ints(500, 1, 5000);
    CHECK(subset_sum(values, 1'000'000));
}

TEST(2, "the target zero is witnessed by no items") {
    const Values values{5, 9};
    const auto witness = subset_sum_witness(values, 0);
    REQUIRE(witness.has_value());
    CHECK(witness->empty());
}

TEST(2, "an unreachable target has no witness") {
    const Values values{2, 4};
    CHECK(!subset_sum_witness(values, 5).has_value());
}

TEST(2, "duplicates can both be used, but only once each") {
    const Values values{6, 6};
    const auto witness = subset_sum_witness(values, 12);
    REQUIRE(witness.has_value());
    CHECK_EQ(*witness, std::vector<std::size_t>{0, 1});
    CHECK(!subset_sum_witness(values, 18).has_value());
}

TEST(2, "random witnesses add up and use each index once") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Values values = random_positive(rng, 14, 25);
        const int target = static_cast<int>(rng.uniform(0, 200));
        INFO("values ", values, " target ", target);
        const auto witness = subset_sum_witness(values, target);
        REQUIRE_EQ(witness.has_value(), subset_sum(values, target));
        if (witness) {
            int sum = 0;
            for (std::size_t k = 0; k < witness->size(); ++k) {
                REQUIRE((*witness)[k] < values.size());
                REQUIRE(k == 0 || (*witness)[k - 1] < (*witness)[k]);
                sum += values[(*witness)[k]];
            }
            REQUIRE_EQ(sum, target);
        }
    }
}

TEST(2, "the witness keeps memory linear in the target") {
    Rng rng;
    const Values values = rng.ints(800, 1, 300);
    const int target = 50'000;
    heap_meter::Scope scope;
    const auto witness = subset_sum_witness(values, target);
    CHECK_LE(scope.peak_bytes(), 32 * static_cast<std::size_t>(target));
    CHECK(witness.has_value());
}

TEST(3, "a new set contains only zero") {
    SumSet set(100);
    CHECK(set.contains(0));
    CHECK(!set.contains(1));
    CHECK_EQ(set.count(), 1);
}

TEST(3, "items compose and each is used once") {
    SumSet set(10);
    set.add(3);
    set.add(4);
    CHECK(set.contains(3));
    CHECK(set.contains(4));
    CHECK(set.contains(7));
    CHECK(!set.contains(6));
    CHECK_EQ(set.count(), 4);
}

TEST(3, "values above the limit and queries above the limit are ignored") {
    SumSet set(8);
    set.add(9);
    CHECK_EQ(set.count(), 1);
    CHECK(!set.contains(9));
    CHECK(!set.contains(1'000'000));
}

TEST(3, "shifting by a multiple of the word size works") {
    SumSet set(300);
    set.add(64);
    CHECK(set.contains(64));
    set.add(128);
    CHECK(set.contains(192));
    CHECK_EQ(set.count(), 4);
}

TEST(3, "sums past the limit do not linger in the last word") {
    SumSet set(70);
    set.add(40);
    set.add(40);  // 80 exceeds the limit
    CHECK(!set.contains(80));
    CHECK_EQ(set.count(), 2);
}

TEST(3, "limits on word boundaries work") {
    for (const std::size_t limit : {63, 64, 127, 128}) {
        INFO("limit ", limit);
        SumSet set(limit);
        for (int value = 1; value <= 300; ++value) {
            set.add(static_cast<std::size_t>(value));
        }
        CHECK_EQ(set.count(), limit + 1);
        CHECK(set.contains(limit));
        CHECK(!set.contains(limit + 1));
    }
}

TEST(3, "random item sequences match a set model at every step") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const auto limit = static_cast<std::size_t>(rng.uniform(0, 500));
        SumSet set(limit);
        std::set<std::size_t> model{0};
        for (int step = 0; step < 25; ++step) {
            const auto value = static_cast<std::size_t>(rng.uniform(0, 200));
            set.add(value);
            std::set<std::size_t> next = model;
            for (const std::size_t sum : model) {
                if (sum + value <= limit) {
                    next.insert(sum + value);
                }
            }
            model = std::move(next);
            INFO("trial ", trial, " step ", step, " limit ", limit);
            REQUIRE_EQ(set.count(), model.size());
            for (std::size_t sum = 0; sum <= limit; sum += 7) {
                REQUIRE_EQ(set.contains(sum), model.contains(sum));
            }
        }
    }
}

TEST(3, "the bitset uses about one bit per sum") {
    const std::size_t limit = 1'000'000;
    heap_meter::Scope scope;
    SumSet set(limit);
    set.add(3);
    CHECK_LE(scope.peak_bytes(), limit / 8 * 2 + 4096);
}

PERF(3, "two thousand items over eight million bits", 2000) {
    Rng rng;
    SumSet set(8'000'000);
    for (int i = 0; i < 2000; ++i) {
        set.add(static_cast<std::size_t>(rng.uniform(1, 40'000)));
    }
    CHECK_GT(set.count(), 1'000'000);
}

TEST(4, "no items split evenly") {
    const auto partition = closest_partition({});
    CHECK_EQ(partition.difference, 0);
    CHECK(partition.left.empty());
}

TEST(4, "a single item has nothing to balance it") {
    const Values values{9};
    CHECK_EQ(closest_partition(values).difference, 9);
}

TEST(4, "an even split is found when it exists") {
    const Values values{3, 1, 4, 2};
    const auto partition = closest_partition(values);
    CHECK_EQ(partition.difference, 0);
    int sum = 0;
    for (const std::size_t i : partition.left) {
        sum += values[i];
    }
    CHECK_EQ(sum, 5);
}

TEST(4, "random inputs match enumeration and the reported side is honest") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        const Values values = random_positive(rng, 14, 40);
        INFO("values ", values);
        const auto partition = closest_partition(values);
        std::int64_t side = 0;
        for (std::size_t k = 0; k < partition.left.size(); ++k) {
            REQUIRE(partition.left[k] < values.size());
            REQUIRE(k == 0 || partition.left[k - 1] < partition.left[k]);
            side += values[partition.left[k]];
        }
        const std::int64_t total = std::accumulate(values.begin(), values.end(), std::int64_t{0});
        REQUIRE_EQ(std::abs(total - 2 * side), partition.difference);
        REQUIRE_EQ(partition.difference, exhaustive_difference(values));
    }
}

PERF(4, "many small jobs balance in pseudo-polynomial time", 2000) {
    Rng rng;
    const Values values = rng.ints(2000, 1, 1000);
    CHECK_LE(closest_partition(values).difference, 1);
}

TEST(5, "the empty subset reaches zero even with negatives") {
    const Values values{-5, -7};
    CHECK(subset_sum_signed(values, 0));
    CHECK(!subset_sum_signed(values, -1));
}

TEST(5, "positives and negatives combine") {
    const Values values{-3, 8};
    CHECK(subset_sum_signed(values, 5));
    CHECK(subset_sum_signed(values, -3));
    CHECK(!subset_sum_signed(values, 6));
}

TEST(5, "a zero value never changes the answer") {
    const Values values{0, 0, 4};
    CHECK(subset_sum_signed(values, 4));
    CHECK(!subset_sum_signed(values, 1));
}

TEST(5, "targets outside the achievable range are rejected quickly") {
    const Values values{1, -1};
    CHECK(!subset_sum_signed(values, 1'000'000'000));
    CHECK(!subset_sum_signed(values, -1'000'000'000));
}

TEST(5, "random signed sets match enumeration") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Values values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 12)), -12, 12);
        const std::int64_t target = rng.uniform(-60, 60);
        INFO("values ", values, " target ", target);
        REQUIRE_EQ(subset_sum_signed(values, target), all_sums(values).contains(target));
    }
}
