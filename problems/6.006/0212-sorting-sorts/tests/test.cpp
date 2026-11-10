#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <numeric>
#include <span>
#include <utility>
#include <vector>

#include "counting_seq.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;

namespace {

struct Blob {
    int key;
    int id;
};

std::vector<const Blob*> pointers_to(const std::vector<Blob>& blobs) {
    std::vector<const Blob*> pointers;
    for (const Blob& blob : blobs) {
        pointers.push_back(&blob);
    }
    return pointers;
}

double comparison_budget(std::size_t n) {
    if (n < 2) {
        return static_cast<double>(n);
    }
    return 1.5 * static_cast<double>(n) * std::ceil(std::log2(static_cast<double>(n))) +
           static_cast<double>(n);
}

}  // namespace

TEST(1, "sorts small sequences") {
    ExpensiveWriteSeq<int> empty({});
    sort_expensive_writes(empty);
    CHECK_EQ(empty.writes(), 0);

    ExpensiveWriteSeq<int> single(std::vector<int>{7});
    sort_expensive_writes(single);
    CHECK_EQ(single.writes(), 0);

    ExpensiveWriteSeq<int> pair(std::vector<int>{2, 1});
    sort_expensive_writes(pair);
    CHECK_EQ(pair.items(), std::vector<int>{1, 2});

    ExpensiveWriteSeq<int> values(std::vector<int>{5, 2, 9, 1, 5, 6});
    sort_expensive_writes(values);
    CHECK_EQ(values.items(), std::vector<int>{1, 2, 5, 5, 6, 9});
}

TEST(1, "writes at most 2(n-1) elements") {
    Rng rng;
    for (std::size_t n : {2U, 3U, 10U, 400U, 2'000U}) {
        for (int round = 0; round < 5; ++round) {
            auto values = rng.ints(n, 0, 1'000);
            auto expected = values;
            std::sort(expected.begin(), expected.end());
            ExpensiveWriteSeq<int> seq(std::move(values));
            sort_expensive_writes(seq);
            INFO("n=", n, " round ", round);
            REQUIRE_EQ(seq.items(), expected);
            REQUIRE(seq.writes() <= 2 * (n - 1));
        }
    }
}

TEST(1, "reads are not limited but writes are") {
    // A reversed sequence is the worst case for writes in most sorts.
    std::vector<int> values(1'000);
    std::iota(values.begin(), values.end(), 0);
    std::reverse(values.begin(), values.end());
    ExpensiveWriteSeq<int> seq(std::move(values));
    sort_expensive_writes(seq);
    CHECK(std::is_sorted(seq.items().begin(), seq.items().end()));
    CHECK_LE(seq.writes(), 2 * 999);
}

TEST(2, "sorts pointers without touching the objects") {
    const std::vector<Blob> blobs = {{3, 0}, {1, 1}, {2, 2}};
    auto pointers = pointers_to(blobs);
    sort_pointers<Blob>(pointers, [](const Blob& a, const Blob& b) { return a.key < b.key; });
    CHECK_EQ(pointers[0]->id, 1);
    CHECK_EQ(pointers[1]->id, 2);
    CHECK_EQ(pointers[2]->id, 0);
}

TEST(2, "keeps exactly the same pointers") {
    Rng rng;
    std::vector<Blob> blobs;
    for (int i = 0; i < 500; ++i) {
        blobs.push_back({static_cast<int>(rng.uniform(0, 30)), i});
    }
    const auto original = pointers_to(blobs);
    auto pointers = original;
    sort_pointers<Blob>(pointers, [](const Blob& a, const Blob& b) { return a.key < b.key; });
    auto sorted_original = original;
    auto sorted_result = pointers;
    std::sort(sorted_original.begin(), sorted_original.end());
    std::sort(sorted_result.begin(), sorted_result.end());
    CHECK_EQ(sorted_result, sorted_original);
}

TEST(2, "edge sizes") {
    std::vector<const Blob*> none;
    sort_pointers<Blob>(none, [](const Blob& a, const Blob& b) { return a.key < b.key; });
    CHECK(none.empty());

    Blob only{1, 0};
    std::vector<const Blob*> one = {&only};
    sort_pointers<Blob>(one, [](const Blob& a, const Blob& b) { return a.key < b.key; });
    CHECK(one[0] == &only);
}

TEST(2, "random keys end up non-decreasing") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(0, 200));
        std::vector<Blob> blobs;
        for (std::size_t i = 0; i < n; ++i) {
            blobs.push_back({static_cast<int>(rng.uniform(0, 20)), static_cast<int>(i)});
        }
        auto pointers = pointers_to(blobs);
        sort_pointers<Blob>(pointers, [](const Blob& a, const Blob& b) { return a.key < b.key; });
        INFO("round ", round);
        REQUIRE(std::is_sorted(pointers.begin(), pointers.end(), [](const Blob* a, const Blob* b) {
            return a->key < b->key;
        }));
    }
}

TEST(2, "comparisons stay within the budget") {
    Rng rng;
    for (std::size_t n : {2U, 9U, 100U, 5'000U, 150'000U}) {
        std::vector<Blob> blobs;
        blobs.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            blobs.push_back({static_cast<int>(rng.uniform(0, 1'000'000)), static_cast<int>(i)});
        }
        auto pointers = pointers_to(blobs);
        std::uint64_t comparisons = 0;
        sort_pointers<Blob>(pointers, [&](const Blob& a, const Blob& b) {
            ++comparisons;
            return a.key < b.key;
        });
        INFO("n=", n);
        CHECK_LE(static_cast<double>(comparisons), comparison_budget(n));
    }
}

TEST(3, "repair sorts small inputs") {
    std::vector<int> empty;
    repair_nearly_sorted<int>(empty);
    CHECK(empty.empty());

    std::vector<int> sorted = {1, 2, 3, 4};
    repair_nearly_sorted<int>(sorted);
    CHECK_EQ(sorted, std::vector<int>{1, 2, 3, 4});

    std::vector<int> one_swap = {1, 3, 2, 4};
    repair_nearly_sorted<int>(one_swap);
    CHECK_EQ(one_swap, std::vector<int>{1, 2, 3, 4});
}

TEST(3, "any permutation still comes out sorted") {
    Rng rng;
    for (int round = 0; round < 800; ++round) {
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 60)), 0, 20);
        auto expected = values;
        std::sort(expected.begin(), expected.end());
        repair_nearly_sorted<int>(values);
        INFO("round ", round);
        REQUIRE_EQ(values, expected);
    }
}

TEST(3, "uses the comparator") {
    std::vector<int> values = {9, 7, 8, 5};
    repair_nearly_sorted<int>(values, std::greater<int>{});
    CHECK_EQ(values, std::vector<int>{9, 8, 7, 5});
}

TEST(3, "comparisons are linear when only a few adjacent swaps happened") {
    Rng rng;
    for (std::size_t n : {1'000U, 20'000U, 200'000U}) {
        for (std::size_t swaps : {0U, 1U, 4U, 200U}) {
            std::vector<int> values(n);
            std::iota(values.begin(), values.end(), 0);
            for (std::size_t s = 0; s < swaps; ++s) {
                const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 2));
                std::swap(values[i], values[i + 1]);
            }
            std::vector<Counted<int>> items;
            items.reserve(n);
            for (int value : values) {
                items.emplace_back(value);
            }
            Counted<int>::comparisons() = 0;
            repair_nearly_sorted<Counted<int>>(items);
            INFO("n=", n, " swaps=", swaps);
            CHECK_LE(Counted<int>::comparisons(), 2 * n + 2 * swaps);
            CHECK(std::is_sorted(items.begin(), items.end()));
        }
    }
}

TEST(3, "comparisons are linear for a truly sorted input of any size") {
    const std::size_t n = 300'000;
    std::vector<Counted<int>> items;
    items.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        items.emplace_back(static_cast<int>(i));
    }
    Counted<int>::comparisons() = 0;
    repair_nearly_sorted<Counted<int>>(items);
    CHECK_LE(Counted<int>::comparisons(), 2 * n);
}

// Selection sort needs 10^12 comparisons here; an O(n log n) sort needs about 3 * 10^7.
PERF(3, "repairing millions of nearly sorted items", 500) {
    std::vector<int> values(3'000'000);
    std::iota(values.begin(), values.end(), 0);
    Rng rng;
    for (int s = 0; s < 50; ++s) {
        const auto i = static_cast<std::size_t>(rng.uniform(0, 2'999'998));
        std::swap(values[i], values[i + 1]);
    }
    repair_nearly_sorted<int>(values);
    CHECK(std::is_sorted(values.begin(), values.end()));
}
