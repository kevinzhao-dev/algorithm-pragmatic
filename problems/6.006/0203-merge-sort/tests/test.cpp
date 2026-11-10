#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "00_alloc_counter.hpp"
#include "00_counters.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;
using practice::Tracked;
using practice6006::AllocationScope;

namespace {

struct Keyed {
    int key;
    int tag;
    friend bool operator==(const Keyed&, const Keyed&) = default;
};

struct NoDefault {
    explicit NoDefault(int v) : value(v) {}
    int value;
    friend bool operator<(const NoDefault& a, const NoDefault& b) { return a.value < b.value; }
};

const auto kByKey = [](const Keyed& a, const Keyed& b) { return a.key < b.key; };

std::vector<Counted<int>> counted(const std::vector<int>& values) {
    std::vector<Counted<int>> out;
    out.reserve(values.size());
    for (int value : values) {
        out.emplace_back(value);
    }
    return out;
}

double comparison_budget(std::size_t n) {
    if (n < 2) {
        return static_cast<double>(n);
    }
    const double levels = std::ceil(std::log2(static_cast<double>(n)));
    return 1.5 * static_cast<double>(n) * levels + static_cast<double>(n);
}

std::uint64_t brute_inversions(const std::vector<int>& values) {
    std::uint64_t count = 0;
    for (std::size_t i = 0; i < values.size(); ++i) {
        for (std::size_t j = i + 1; j < values.size(); ++j) {
            count += values[i] > values[j] ? 1 : 0;
        }
    }
    return count;
}

std::uint64_t brute_kendall(const std::vector<int>& a, const std::vector<int>& b) {
    const std::size_t n = a.size();
    std::vector<int> pos_a(n);
    std::vector<int> pos_b(n);
    for (std::size_t i = 0; i < n; ++i) {
        pos_a[static_cast<std::size_t>(a[i])] = static_cast<int>(i);
        pos_b[static_cast<std::size_t>(b[i])] = static_cast<int>(i);
    }
    std::uint64_t count = 0;
    for (std::size_t x = 0; x < n; ++x) {
        for (std::size_t y = x + 1; y < n; ++y) {
            const bool order_a = pos_a[x] < pos_a[y];
            const bool order_b = pos_b[x] < pos_b[y];
            count += order_a != order_b ? 1 : 0;
        }
    }
    return count;
}

}  // namespace

TEST(1, "merging two sorted lists") {
    const std::vector<int> a = {1, 4, 6};
    const std::vector<int> b = {2, 3, 7};
    CHECK_EQ(merge_sorted<int>(a, b), std::vector<int>{1, 2, 3, 4, 6, 7});
}

TEST(1, "merging with an empty side") {
    const std::vector<int> a = {1, 2};
    const std::vector<int> empty;
    CHECK_EQ(merge_sorted<int>(a, empty), a);
    CHECK_EQ(merge_sorted<int>(empty, a), a);
    CHECK(merge_sorted<int>(empty, empty).empty());
}

TEST(1, "merge is stable: ties come from the first list first") {
    const std::vector<Keyed> a = {{1, 10}, {2, 11}};
    const std::vector<Keyed> b = {{1, 20}, {2, 21}};
    const std::vector<Keyed> merged = merge_sorted<Keyed>(a, b, kByKey);
    CHECK(merged == std::vector<Keyed>({{1, 10}, {1, 20}, {2, 11}, {2, 21}}));
}

TEST(1, "merge uses at most |a| + |b| comparisons") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        auto a = rng.ints(static_cast<std::size_t>(rng.uniform(0, 200)), 0, 400);
        auto b = rng.ints(static_cast<std::size_t>(rng.uniform(0, 200)), 0, 400);
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());
        auto expected = a;
        expected.insert(expected.end(), b.begin(), b.end());
        std::stable_sort(expected.begin(), expected.end());
        const auto ca = counted(a);
        const auto cb = counted(b);
        Counted<int>::comparisons() = 0;
        const auto merged = merge_sorted<Counted<int>>(ca, cb);
        INFO("round ", round);
        REQUIRE(Counted<int>::comparisons() <= a.size() + b.size());
        std::vector<int> values;
        for (const auto& item : merged) {
            values.push_back(item.value());
        }
        REQUIRE_EQ(values, expected);
    }
}

TEST(2, "merge_sort basics") {
    std::vector<int> empty;
    merge_sort<int>(empty);
    CHECK(empty.empty());

    std::vector<int> single = {3};
    merge_sort<int>(single);
    CHECK_EQ(single, std::vector<int>{3});

    std::vector<int> values = {5, 2, 9, 1, 5, 6};
    merge_sort<int>(values);
    CHECK_EQ(values, std::vector<int>{1, 2, 5, 5, 6, 9});

    std::vector<int> reversed = {4, 3, 2, 1, 0};
    merge_sort<int>(reversed);
    CHECK_EQ(reversed, std::vector<int>{0, 1, 2, 3, 4});
}

TEST(2, "merge_sort uses the comparator") {
    std::vector<int> values = {3, 1, 2};
    merge_sort<int>(values, std::greater<int>{});
    CHECK_EQ(values, std::vector<int>{3, 2, 1});
}

TEST(2, "merge_sort is stable") {
    Rng rng;
    for (int round = 0; round < 800; ++round) {
        std::vector<Keyed> records;
        const auto n = static_cast<std::size_t>(rng.uniform(0, 100));
        for (std::size_t i = 0; i < n; ++i) {
            records.push_back({static_cast<int>(rng.uniform(0, 6)), static_cast<int>(i)});
        }
        auto expected = records;
        std::stable_sort(expected.begin(), expected.end(), kByKey);
        merge_sort<Keyed>(records, kByKey);
        INFO("round ", round);
        REQUIRE(records == expected);
    }
}

TEST(2, "random arrays match std::stable_sort") {
    Rng rng;
    for (int round = 0; round < 1'000; ++round) {
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 300)), -50, 50);
        auto expected = values;
        std::stable_sort(expected.begin(), expected.end());
        merge_sort<int>(values);
        INFO("round ", round);
        REQUIRE_EQ(values, expected);
    }
}

TEST(2, "merge_sort needs only movable elements") {
    std::vector<std::unique_ptr<int>> pointers;
    for (int value : {5, 1, 4, 2, 3}) {
        pointers.push_back(std::make_unique<int>(value));
    }
    merge_sort<std::unique_ptr<int>>(pointers,
                                     [](const auto& a, const auto& b) { return *a < *b; });
    std::vector<int> values;
    for (const auto& p : pointers) {
        values.push_back(*p);
    }
    CHECK_EQ(values, std::vector<int>{1, 2, 3, 4, 5});

    std::vector<NoDefault> items;
    for (int value : {3, 1, 2}) {
        items.emplace_back(value);
    }
    merge_sort<NoDefault>(items);
    CHECK_EQ(items[0].value, 1);
}

TEST(2, "comparisons stay within n log n") {
    Rng rng;
    for (std::size_t n : {2U, 3U, 7U, 16U, 1'000U, 100'000U}) {
        const auto items = counted(rng.ints(n, 0, 1'000'000));
        auto copy = items;
        Counted<int>::comparisons() = 0;
        merge_sort<Counted<int>>(copy);
        INFO("n=", n);
        CHECK_LE(static_cast<double>(Counted<int>::comparisons()), comparison_budget(n));
        CHECK(std::is_sorted(copy.begin(), copy.end()));
    }
}

TEST(2, "already sorted input is not quadratic") {
    const std::size_t n = 200'000;
    std::vector<int> values(n);
    std::iota(values.begin(), values.end(), 0);
    auto items = counted(values);
    Counted<int>::comparisons() = 0;
    merge_sort<Counted<int>>(items);
    CHECK_LE(static_cast<double>(Counted<int>::comparisons()), comparison_budget(n));
}

TEST(2, "temporary storage is allocated only once") {
    Rng rng;
    auto values = rng.ints(50'000, 0, 1'000'000);
    const AllocationScope scope;
    merge_sort<int>(values);
    CHECK_LE(scope.allocations(), 2);
    CHECK(std::is_sorted(values.begin(), values.end()));
}

TEST(2, "elements are not leaked") {
    const auto before = Tracked::live();
    {
        Rng rng;
        std::vector<Tracked> items;
        for (int value : rng.ints(2'000, 0, 500)) {
            items.emplace_back(value);
        }
        merge_sort<Tracked>(items, [](const Tracked& a, const Tracked& b) {
            return a.value() < b.value();
        });
    }
    CHECK_EQ(Tracked::live(), before);
}

// A quadratic sort needs 10^12 comparisons here.
PERF(2, "sorting a million items", 2500) {
    Rng rng;
    auto values = rng.ints(1'000'000, 0, 1'000'000'000);
    merge_sort<int>(values);
    CHECK(std::is_sorted(values.begin(), values.end()));
}

TEST(3, "inversions of simple arrays") {
    const std::vector<int> sorted = {1, 2, 3};
    CHECK_EQ(count_inversions(sorted), 0);
    const std::vector<int> reversed = {3, 2, 1};
    CHECK_EQ(count_inversions(reversed), 3);
    const std::vector<int> mixed = {2, 4, 1, 3, 5};
    CHECK_EQ(count_inversions(mixed), 3);
    const std::vector<int> empty;
    CHECK_EQ(count_inversions(empty), 0);
    const std::vector<int> ties = {2, 2, 2};
    CHECK_EQ(count_inversions(ties), 0);
}

TEST(3, "inversions use the full 64-bit range") {
    const std::size_t n = 200'000;
    std::vector<int> reversed(n);
    std::iota(reversed.begin(), reversed.end(), 0);
    std::reverse(reversed.begin(), reversed.end());
    const std::uint64_t expected = static_cast<std::uint64_t>(n) * (n - 1) / 2;
    CHECK_EQ(count_inversions(reversed), expected);
}

TEST(3, "count_inversions does not modify its input") {
    const std::vector<int> values = {3, 1, 2};
    const auto copy = values;
    count_inversions(values);
    CHECK_EQ(values, copy);
}

TEST(3, "random arrays match a quadratic count") {
    Rng rng;
    for (int round = 0; round < 1'500; ++round) {
        const auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 200)), 0, 30);
        INFO("round ", round);
        REQUIRE_EQ(count_inversions(values), brute_inversions(values));
    }
}

TEST(3, "kendall tau of identical and reversed rankings") {
    const std::vector<int> identity = {0, 1, 2, 3, 4};
    std::vector<int> reversed = identity;
    std::reverse(reversed.begin(), reversed.end());
    CHECK_EQ(kendall_tau_distance(identity, identity), 0);
    CHECK_EQ(kendall_tau_distance(identity, reversed), 10);
    CHECK_EQ(kendall_tau_distance(reversed, identity), 10);
}

TEST(3, "kendall tau counts pairs ordered differently") {
    const std::vector<int> a = {2, 0, 1};
    const std::vector<int> b = {0, 1, 2};
    CHECK_EQ(kendall_tau_distance(a, b), brute_kendall(a, b));
    CHECK_EQ(kendall_tau_distance(a, b), 2);
}

TEST(3, "kendall tau of empty and single-element rankings") {
    const std::vector<int> empty;
    CHECK_EQ(kendall_tau_distance(empty, empty), 0);
    const std::vector<int> single = {0};
    CHECK_EQ(kendall_tau_distance(single, single), 0);
}

TEST(3, "kendall tau rejects bad input") {
    const std::vector<int> ok = {0, 1, 2};
    const std::vector<int> repeated = {0, 0, 2};
    const std::vector<int> out_of_range = {0, 1, 3};
    const std::vector<int> shorter = {0, 1};
    CHECK_THROWS_AS(kendall_tau_distance(ok, shorter), std::invalid_argument);
    CHECK_THROWS_AS(kendall_tau_distance(ok, repeated), std::invalid_argument);
    CHECK_THROWS_AS(kendall_tau_distance(repeated, ok), std::invalid_argument);
    CHECK_THROWS_AS(kendall_tau_distance(ok, out_of_range), std::invalid_argument);
}

TEST(3, "random permutations match a brute-force distance") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(0, 150));
        const auto a = rng.permutation(n);
        const auto b = rng.permutation(n);
        INFO("round ", round);
        REQUIRE_EQ(kendall_tau_distance(a, b), brute_kendall(a, b));
    }
}

// An O(n^2) count needs 10^12 comparisons here.
PERF(3, "counting inversions of a million elements", 2500) {
    Rng rng;
    const auto values = rng.ints(1'000'000, 0, 1'000'000'000);
    CHECK_GT(count_inversions(values), 0);
}
