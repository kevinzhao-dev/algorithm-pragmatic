#include <algorithm>
#include <cstdint>
#include <functional>
#include <set>
#include <span>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;
using practice::Tracked;

namespace {

int ceil_lg(std::size_t n) {
    int bits = 0;
    while ((std::size_t{1} << bits) < n) {
        ++bits;
    }
    return bits;
}

using IntGarden = Garden<int>;

// Accepts any tie-break: the multiset of returned scores must match the true top k.
void check_top_k(const std::vector<IntGarden>& gardens, const std::vector<int>& ids, std::size_t k) {
    REQUIRE_EQ(ids.size(), k);
    std::vector<int> expected;
    for (const IntGarden& garden : gardens) {
        expected.push_back(garden.score);
    }
    std::sort(expected.rbegin(), expected.rend());
    expected.resize(k);

    std::vector<int> got;
    std::set<int> seen;
    for (int id : ids) {
        REQUIRE(seen.insert(id).second);
        const auto it = std::find_if(gardens.begin(), gardens.end(), [&](const IntGarden& g) { return g.id == id; });
        REQUIRE(it != gardens.end());
        got.push_back(it->score);
    }
    std::sort(got.rbegin(), got.rend());
    CHECK_EQ(got, expected);
}

}  // namespace

TEST(1, "returns the highest scores") {
    const std::vector<IntGarden> gardens{{5, 1}, {9, 2}, {1, 3}, {7, 4}, {3, 5}};
    const auto ids = top_k_ids(std::span<const IntGarden>(gardens), 2);
    CHECK_EQ(std::set<int>(ids.begin(), ids.end()), std::set<int>{2, 4});
}

TEST(1, "k of zero and k equal to n") {
    const std::vector<IntGarden> gardens{{5, 1}, {9, 2}, {1, 3}};
    CHECK(top_k_ids(std::span<const IntGarden>(gardens), 0).empty());
    const auto ids = top_k_ids(std::span<const IntGarden>(gardens), 3);
    CHECK_EQ(std::set<int>(ids.begin(), ids.end()), std::set<int>{1, 2, 3});
}

TEST(1, "an empty input with k of zero") {
    const std::vector<IntGarden> gardens;
    CHECK(top_k_ids(std::span<const IntGarden>(gardens), 0).empty());
}

TEST(1, "ties may be broken either way") {
    const std::vector<IntGarden> gardens{{7, 1}, {7, 2}, {7, 3}, {1, 4}};
    const auto ids = top_k_ids(std::span<const IntGarden>(gardens), 2);
    check_top_k(gardens, ids, 2);
}

TEST(1, "random inputs return a correct top k") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 300));
        const auto k = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n)));
        std::vector<IntGarden> gardens;
        for (std::size_t i = 0; i < n; ++i) {
            gardens.push_back({static_cast<int>(rng.uniform(0, 50)), static_cast<int>(i)});
        }
        check_top_k(gardens, top_k_ids(std::span<const IntGarden>(gardens), k), k);
    }
}

TEST(1, "comparison count beats sorting when k is small") {
    Rng rng;
    const std::size_t n = 200'000;
    const std::size_t k = 50;
    std::vector<Garden<Counted<int>>> gardens;
    for (std::size_t i = 0; i < n; ++i) {
        gardens.push_back({Counted<int>(static_cast<int>(rng.uniform(0, 1'000'000'000))), static_cast<int>(i)});
    }
    Counted<int>::comparisons() = 0;
    const auto ids = top_k_ids(std::span<const Garden<Counted<int>>>(gardens), k);
    CHECK_EQ(ids.size(), k);
    CHECK_LE(Counted<int>::comparisons(), static_cast<std::uint64_t>(3 * n + 4 * k * static_cast<std::size_t>(ceil_lg(n))));
}

PERF(1, "ten million items, top one hundred", 2500) {
    Rng rng;
    std::vector<IntGarden> gardens(10'000'000);
    for (std::size_t i = 0; i < gardens.size(); ++i) {
        gardens[i] = {static_cast<int>(rng.uniform(0, 2'000'000'000)), static_cast<int>(i)};
    }
    const auto ids = top_k_ids(std::span<const IntGarden>(gardens), 100);
    CHECK_EQ(ids.size(), 100);
}

TEST(2, "only ids above the threshold come back") {
    std::vector<IntGarden> heap{{9, 1}, {7, 2}, {8, 3}, {3, 4}, {5, 5}, {6, 6}, {2, 7}};
    const auto below = [&](int x) {
        const auto ids = ids_above(std::span<const IntGarden>(heap), x);
        return std::set<int>(ids.begin(), ids.end());
    };
    CHECK_EQ(below(6), std::set<int>{1, 2, 3});
    CHECK_EQ(below(0), std::set<int>{1, 2, 3, 4, 5, 6, 7});
    CHECK_EQ(below(9), std::set<int>{});
    CHECK_EQ(below(100), std::set<int>{});
}

TEST(2, "equal scores are excluded when equal to the threshold") {
    std::vector<IntGarden> heap{{5, 1}, {5, 2}, {5, 3}};
    CHECK(ids_above(std::span<const IntGarden>(heap), 5).empty());
    CHECK_EQ(ids_above(std::span<const IntGarden>(heap), 4).size(), 3);
}

TEST(2, "an empty heap has no answers") {
    std::vector<IntGarden> heap;
    CHECK(ids_above(std::span<const IntGarden>(heap), 0).empty());
}

TEST(2, "random heaps match a direct filter") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 400));
        std::vector<IntGarden> heap;
        for (std::size_t i = 0; i < n; ++i) {
            heap.push_back({static_cast<int>(rng.uniform(0, 200)), static_cast<int>(i)});
        }
        std::make_heap(heap.begin(), heap.end(), [](const IntGarden& a, const IntGarden& b) { return a.score < b.score; });
        const int threshold = static_cast<int>(rng.uniform(-5, 205));
        std::set<int> expected;
        for (const IntGarden& garden : heap) {
            if (garden.score > threshold) {
                expected.insert(garden.id);
            }
        }
        const auto got = ids_above(std::span<const IntGarden>(heap), threshold);
        CHECK_EQ(std::set<int>(got.begin(), got.end()), expected);
        CHECK_EQ(got.size(), expected.size());
    }
}

TEST(2, "visits only O(k) nodes") {
    Rng rng;
    const std::size_t n = 1 << 18;
    std::vector<Garden<Counted<int>>> heap;
    for (std::size_t i = 0; i < n; ++i) {
        heap.push_back({Counted<int>(static_cast<int>(rng.uniform(0, 1'000'000'000))), static_cast<int>(i)});
    }
    std::make_heap(heap.begin(), heap.end(),
                   [](const auto& a, const auto& b) { return a.score < b.score; });
    for (int round = 0; round < 40; ++round) {
        INFO("round ", round);
        const auto answers = static_cast<std::size_t>(rng.uniform(0, 300));
        // A high threshold leaves only `answers` results near the top of the heap.
        std::vector<int> values;
        for (const auto& garden : heap) {
            values.push_back(garden.score.value());
        }
        std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(answers), values.end(), std::greater<int>{});
        const Counted<int> threshold(values[answers]);
        Counted<int>::comparisons() = 0;
        const auto got = ids_above(std::span<const Garden<Counted<int>>>(heap), threshold);
        CHECK_EQ(got.size(), answers);
        CHECK_LE(Counted<int>::comparisons(), static_cast<std::uint64_t>(3 * answers + 8));
    }
}

TEST(3, "keeps the largest values seen so far") {
    TopKTracker<int> tracker(3);
    for (int value : {5, 1, 9, 7, 3}) {
        tracker.offer(value);
    }
    CHECK_EQ(tracker.size(), 3);
    CHECK_EQ(tracker.sorted_largest(), std::vector<int>{9, 7, 5});
}

TEST(3, "threshold is null until the tracker is full") {
    TopKTracker<int> tracker(2);
    CHECK(tracker.threshold() == nullptr);
    tracker.offer(1);
    CHECK(tracker.threshold() == nullptr);
    tracker.offer(5);
    REQUIRE(tracker.threshold() != nullptr);
    CHECK_EQ(*tracker.threshold(), 1);
    tracker.offer(3);
    CHECK_EQ(*tracker.threshold(), 3);
    tracker.offer(2);
    CHECK_EQ(*tracker.threshold(), 3);  // 2 could not enter
}

TEST(3, "k of zero keeps nothing") {
    TopKTracker<int> tracker(0);
    tracker.offer(1);
    tracker.offer(9);
    CHECK_EQ(tracker.size(), 0);
    CHECK(tracker.threshold() == nullptr);
    CHECK(tracker.sorted_largest().empty());
}

TEST(3, "a custom comparator ranks by a different notion of large") {
    TopKTracker<int, std::greater<int>> tracker(2, std::greater<int>{});
    for (int value : {5, 1, 9, 2}) {
        tracker.offer(value);
    }
    CHECK_EQ(tracker.sorted_largest(), std::vector<int>{1, 2});
}

TEST(3, "duplicates count separately") {
    TopKTracker<int> tracker(3);
    for (int value : {5, 5, 5, 5, 1}) {
        tracker.offer(value);
    }
    CHECK_EQ(tracker.sorted_largest(), std::vector<int>{5, 5, 5});
}

TEST(3, "random streams match sorting the prefix") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        INFO("round ", round);
        const auto k = static_cast<std::size_t>(rng.uniform(0, 40));
        TopKTracker<int> tracker(k);
        std::vector<int> stream;
        for (int step = 0; step < 300; ++step) {
            const int value = static_cast<int>(rng.uniform(-100, 100));
            tracker.offer(value);
            stream.push_back(value);
            auto expected = stream;
            std::sort(expected.rbegin(), expected.rend());
            if (expected.size() > k) {
                expected.resize(k);
            }
            REQUIRE_EQ(tracker.sorted_largest(), expected);
            REQUIRE_EQ(tracker.size(), expected.size());
            if (expected.size() == k && k != 0) {
                REQUIRE(tracker.threshold() != nullptr);
                REQUIRE_EQ(*tracker.threshold(), expected.back());
            } else {
                REQUIRE(tracker.threshold() == nullptr);
            }
        }
    }
}

TEST(3, "memory is bounded by k") {
    Rng rng;
    const auto before = Tracked::live();
    struct Less {
        bool operator()(const Tracked& a, const Tracked& b) const { return a.value() < b.value(); }
    };
    {
        TopKTracker<Tracked, Less> tracker(50);
        for (int i = 0; i < 100'000; ++i) {
            tracker.offer(Tracked(static_cast<int>(rng.uniform(0, 1'000'000))));
            REQUIRE(Tracked::live() <= before + 50 + 2);
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(3, "offer costs O(log k) comparisons and rejects cheaply") {
    Rng rng;
    const std::size_t k = 1000;
    TopKTracker<Counted<int>> tracker(k);
    for (std::size_t i = 0; i < k; ++i) {
        tracker.offer(Counted<int>(1'000'000));
    }
    Counted<int>::comparisons() = 0;
    const std::size_t rejected = 100'000;
    for (std::size_t i = 0; i < rejected; ++i) {
        tracker.offer(Counted<int>(static_cast<int>(rng.uniform(0, 999'999))));
    }
    CHECK_LE(Counted<int>::comparisons(), rejected + rejected / 10);

    Counted<int>::comparisons() = 0;
    const std::size_t accepted = 20'000;
    for (std::size_t i = 0; i < accepted; ++i) {
        tracker.offer(Counted<int>(1'000'001 + static_cast<int>(i)));
    }
    CHECK_LE(Counted<int>::comparisons(), accepted * (3 * static_cast<std::size_t>(ceil_lg(k + 1)) + 6));
}

PERF(3, "fifty million offers into a small tracker", 2500) {
    Rng rng;
    TopKTracker<int> tracker(100);
    for (int i = 0; i < 50'000'000; ++i) {
        tracker.offer(static_cast<int>(rng.uniform(0, 2'000'000'000)));
    }
    CHECK_EQ(tracker.size(), 100);
}
