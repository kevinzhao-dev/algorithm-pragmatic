#include <algorithm>
#include <cstddef>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "00_alloc_counter.hpp"
#include "00_counters.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;
using practice6006::AllocationScope;
using practice6006::Probe;

namespace {

struct Keyed {
    int key;
    int tag;
    friend bool operator==(const Keyed&, const Keyed&) = default;
};

std::vector<Probe<int>> probes_of(const std::vector<int>& values) {
    std::vector<Probe<int>> out;
    out.reserve(values.size());
    for (int value : values) {
        out.emplace_back(value);
    }
    return out;
}

std::vector<int> plain_values(const std::vector<Probe<int>>& probes) {
    std::vector<int> out;
    for (const auto& probe : probes) {
        out.push_back(probe.value());
    }
    return out;
}

std::uint64_t count_inversions(const std::vector<int>& values) {
    std::uint64_t inversions = 0;
    for (std::size_t i = 0; i < values.size(); ++i) {
        for (std::size_t j = i + 1; j < values.size(); ++j) {
            inversions += values[j] < values[i] ? 1 : 0;
        }
    }
    return inversions;
}

// An array of n items made `swaps` random adjacent swaps away from sorted order.
std::vector<int> nearly_sorted(Rng& rng, std::size_t n, std::size_t swaps) {
    std::vector<int> values(n);
    for (std::size_t i = 0; i < n; ++i) {
        values[i] = static_cast<int>(i);
    }
    for (std::size_t s = 0; s < swaps && n > 1; ++s) {
        const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 2));
        std::swap(values[i], values[i + 1]);
    }
    return values;
}

template <typename Sort>
void check_sort_basics(Sort sort) {
    std::vector<int> empty;
    sort(std::span<int>(empty));
    CHECK(empty.empty());

    std::vector<int> single = {42};
    sort(std::span<int>(single));
    CHECK_EQ(single, std::vector<int>{42});

    std::vector<int> pair = {2, 1};
    sort(std::span<int>(pair));
    CHECK_EQ(pair, std::vector<int>{1, 2});

    std::vector<int> sorted = {1, 2, 3, 4};
    sort(std::span<int>(sorted));
    CHECK_EQ(sorted, std::vector<int>{1, 2, 3, 4});

    std::vector<int> reversed = {5, 4, 3, 2, 1};
    sort(std::span<int>(reversed));
    CHECK_EQ(reversed, std::vector<int>{1, 2, 3, 4, 5});

    std::vector<int> duplicates = {3, 1, 2, 3, 1};
    sort(std::span<int>(duplicates));
    CHECK_EQ(duplicates, std::vector<int>{1, 1, 2, 3, 3});
}

template <typename Sort>
void check_random_sort(Sort sort) {
    Rng rng;
    for (int round = 0; round < 1'500; ++round) {
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 60)), -20, 20);
        auto expected = values;
        std::stable_sort(expected.begin(), expected.end());
        sort(std::span<int>(values));
        INFO("round ", round);
        REQUIRE_EQ(values, expected);
    }
}

}  // namespace

TEST(1, "selection_sort basics") {
    check_sort_basics([](std::span<int> items) { selection_sort<int>(items); });
}

TEST(1, "selection_sort works with a custom comparator") {
    std::vector<int> values = {2, 9, 4};
    selection_sort<int>(values, std::greater<int>{});
    CHECK_EQ(values, std::vector<int>{9, 4, 2});
}

TEST(1, "selection_sort sorts structs and strings by moving them") {
    std::vector<std::string> words = {"pear", "apple", "fig"};
    selection_sort<std::string>(words);
    CHECK_EQ(words, std::vector<std::string>{"apple", "fig", "pear"});
    std::vector<Keyed> records = {{3, 0}, {1, 1}, {2, 2}};
    selection_sort<Keyed>(records, [](const Keyed& a, const Keyed& b) { return a.key < b.key; });
    CHECK_EQ(records[0].tag, 1);
}

TEST(1, "selection_sort writes at most 3(n-1) elements") {
    using Item = Probe<int>;
    Rng rng;
    for (int n : {0, 1, 2, 3, 10, 500, 3'000}) {
        auto items = probes_of(rng.ints(static_cast<std::size_t>(n), 0, 100'000));
        Item::reset();
        selection_sort<Item>(items);
        INFO("n=", n);
        CHECK_LE(Item::writes(), n == 0 ? 0 : 3 * (n - 1));
        CHECK(std::is_sorted(items.begin(), items.end()));
    }
}

TEST(1, "selection_sort sorts in place without allocating") {
    Rng rng;
    auto values = rng.ints(1'000, 0, 50);
    const AllocationScope scope;
    selection_sort<int>(values);
    CHECK_EQ(scope.allocations(), 0);
    CHECK(std::is_sorted(values.begin(), values.end()));
}

TEST(1, "random arrays match std::stable_sort") {
    check_random_sort([](std::span<int> items) { selection_sort<int>(items); });
}

TEST(2, "insertion_sort basics") {
    check_sort_basics([](std::span<int> items) { insertion_sort<int>(items); });
}

TEST(2, "insertion_sort uses the comparator") {
    std::vector<int> values = {5, 1, 7};
    insertion_sort<int>(values, std::greater<int>{});
    CHECK_EQ(values, std::vector<int>{7, 5, 1});
}

TEST(2, "insertion_sort is stable") {
    Rng rng;
    for (int round = 0; round < 800; ++round) {
        std::vector<Keyed> records;
        const auto n = static_cast<std::size_t>(rng.uniform(0, 40));
        for (std::size_t i = 0; i < n; ++i) {
            records.push_back({static_cast<int>(rng.uniform(0, 5)), static_cast<int>(i)});
        }
        auto expected = records;
        std::stable_sort(expected.begin(), expected.end(),
                         [](const Keyed& a, const Keyed& b) { return a.key < b.key; });
        insertion_sort<Keyed>(records, [](const Keyed& a, const Keyed& b) { return a.key < b.key; });
        INFO("round ", round);
        REQUIRE(records == expected);
    }
}

TEST(2, "insertion_sort sorts in place without allocating") {
    Rng rng;
    auto values = rng.ints(1'000, 0, 50);
    const AllocationScope scope;
    insertion_sort<int>(values);
    CHECK_EQ(scope.allocations(), 0);
    CHECK(std::is_sorted(values.begin(), values.end()));
}

TEST(2, "random arrays match std::stable_sort") {
    check_random_sort([](std::span<int> items) { insertion_sort<int>(items); });
}

TEST(2, "work is adaptive to the number of inversions") {
    using Item = Probe<int>;
    Rng rng;
    for (std::size_t swaps : {0U, 1U, 10U, 100U, 10'000U}) {
        for (std::size_t n : {2'000U, 8'000U}) {
            const auto values = nearly_sorted(rng, n, swaps);
            const std::uint64_t inversions = count_inversions(values);
            auto items = probes_of(values);
            Item::reset();
            insertion_sort<Item>(items);
            INFO("n=", n, " swaps=", swaps, " inversions=", inversions);
            CHECK_LE(Item::compares(), 2 * n + inversions);
            CHECK_LE(Item::writes(), 3 * (n + inversions));
            CHECK(std::is_sorted(items.begin(), items.end()));
            CHECK_EQ(plain_values(items).size(), n);
        }
    }
}

TEST(2, "already sorted input takes a linear number of comparisons") {
    using Item = Probe<int>;
    const std::size_t n = 200'000;
    std::vector<Item> items;
    items.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        items.emplace_back(static_cast<int>(i));
    }
    Item::reset();
    insertion_sort<Item>(items);
    CHECK_LE(Item::compares(), 2 * n);
    CHECK_EQ(Item::writes(), 0);
}

// A quadratic sort here would need 20 billion comparisons.
PERF(2, "nearly sorted input scales linearly", 1500) {
    Rng rng;
    auto values = nearly_sorted(rng, 2'000'000, 1'000);
    const std::size_t n = values.size();
    insertion_sort<int>(values);
    CHECK_EQ(values.front(), 0);
    CHECK_EQ(values.back(), static_cast<int>(n) - 1);
}

TEST(3, "k must be positive") {
    CHECK_THROWS_AS(TopK<int>(0), std::invalid_argument);
}

TEST(3, "fewer items than k are all kept, largest first") {
    TopK<int> top(5);
    top.push(1);
    top.push(9);
    top.push(4);
    CHECK_EQ(top.size(), 3);
    CHECK_EQ(std::vector<int>(top.items().begin(), top.items().end()), std::vector<int>{9, 4, 1});
}

TEST(3, "keeps the k largest and evicts the smallest") {
    TopK<int> top(3);
    for (int value : {5, 1, 9, 3, 7, 2, 8}) {
        top.push(value);
    }
    CHECK_EQ(top.size(), 3);
    CHECK_EQ(std::vector<int>(top.items().begin(), top.items().end()), std::vector<int>{9, 8, 7});
}

TEST(3, "a value not above the current k-th largest is dropped") {
    TopK<int> top(2);
    top.push(10);
    top.push(5);
    top.push(5);
    top.push(4);
    CHECK_EQ(std::vector<int>(top.items().begin(), top.items().end()), std::vector<int>{10, 5});
}

TEST(3, "k = 1 keeps the maximum") {
    TopK<int> top(1);
    for (int value : {3, 9, 1, 12, 0}) {
        top.push(value);
    }
    CHECK_EQ(std::vector<int>(top.items().begin(), top.items().end()), std::vector<int>{12});
}

TEST(3, "equal values keep their arrival order") {
    using Pair = std::pair<int, int>;
    const auto by_first = [](const Pair& a, const Pair& b) { return a.first < b.first; };
    TopK<Pair, decltype(by_first)> top(4, by_first);
    top.push({5, 0});
    top.push({7, 1});
    top.push({5, 2});
    top.push({7, 3});
    top.push({9, 4});
    const std::vector<Pair> expected = {{9, 4}, {7, 1}, {7, 3}, {5, 0}};
    CHECK_EQ(std::vector<Pair>(top.items().begin(), top.items().end()), expected);
}

TEST(3, "a custom comparator flips the meaning of largest") {
    TopK<int, std::greater<int>> smallest(2);
    for (int value : {5, 2, 8, 1, 9}) {
        smallest.push(value);
    }
    CHECK_EQ(std::vector<int>(smallest.items().begin(), smallest.items().end()), std::vector<int>{1, 2});
}

TEST(3, "random streams match a sorted model") {
    Rng rng;
    for (int round = 0; round < 1'000; ++round) {
        const auto k = static_cast<std::size_t>(rng.uniform(1, 12));
        const auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 80)), 0, 30);
        TopK<int> top(k);
        for (int value : values) {
            top.push(value);
        }
        auto expected = values;
        std::sort(expected.begin(), expected.end(), std::greater<int>{});
        expected.resize(std::min(k, expected.size()));
        INFO("round ", round);
        REQUIRE_EQ(std::vector<int>(top.items().begin(), top.items().end()), expected);
    }
}

TEST(3, "each push moves at most k + 8 elements") {
    using Item = Probe<int>;
    Rng rng;
    const std::size_t k = 500;
    TopK<Item> top(k);
    const auto values = rng.ints(20'000, 0, 1'000'000);
    Item::reset();
    for (int value : values) {
        Item::reset();
        top.push(Item(value));
        INFO("value=", value);
        REQUIRE(Item::writes() <= k + 8);
    }
}

TEST(3, "items are released") {
    const auto before = Tracked::live();
    {
        const auto by_value = [](const Tracked& a, const Tracked& b) { return a.value() < b.value(); };
        TopK<Tracked, decltype(by_value)> top(5, by_value);
        for (int i = 0; i < 100; ++i) {
            top.push(Tracked(i));
        }
        CHECK_EQ(top.size(), 5);
    }
    CHECK_EQ(Tracked::live(), before);
}
