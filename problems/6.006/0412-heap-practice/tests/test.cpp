#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;

namespace {

bool is_min_heap(std::span<const int> values) {
    for (std::size_t i = 1; i < values.size(); ++i) {
        if (values[(i - 1) / 2] > values[i]) {
            return false;
        }
    }
    return true;
}

void check_trace(std::vector<int> input) {
    const std::vector<int> original = input;
    const auto trace = min_heapify_trace(std::span<int>(input));
    CHECK(is_min_heap(input));
    auto sorted_a = input;
    auto sorted_b = original;
    std::sort(sorted_a.begin(), sorted_a.end());
    std::sort(sorted_b.begin(), sorted_b.end());
    CHECK_EQ(sorted_a, sorted_b);
    CHECK_LE(trace.size(), original.size());

    std::vector<int> replay = original;
    for (const auto& [i, j] : trace) {
        REQUIRE(i < replay.size() && j < replay.size());
        REQUIRE(i == (j - 1) / 2 || j == (i - 1) / 2);
        REQUIRE(i != j);
        std::swap(replay[i], replay[j]);
    }
    CHECK_EQ(replay, input);
}

}  // namespace

TEST(1, "recognises max-heaps and min-heaps") {
    CHECK(classify_heap(std::vector<int>{9, 8, 7, 3, 5}) == HeapKind::Max);
    CHECK(classify_heap(std::vector<int>{1, 2, 3, 5, 4}) == HeapKind::Min);
}

TEST(1, "neither when both properties break") {
    CHECK(classify_heap(std::vector<int>{3, 9, 1}) == HeapKind::Neither);
    CHECK(classify_heap(std::vector<int>{5, 6, 2, 1}) == HeapKind::Neither);
}

TEST(1, "empty and single-element arrays are both") {
    CHECK(classify_heap(std::vector<int>{}) == HeapKind::Both);
    CHECK(classify_heap(std::vector<int>{7}) == HeapKind::Both);
    CHECK(classify_heap(std::vector<int>{4, 4, 4}) == HeapKind::Both);
}

TEST(1, "equal children are allowed") {
    CHECK(classify_heap(std::vector<int>{5, 5, 3}) == HeapKind::Max);
    CHECK(classify_heap(std::vector<int>{2, 2, 5}) == HeapKind::Min);
}

TEST(1, "random arrays match a direct property check") {
    Rng rng;
    for (int round = 0; round < 2000; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 12));
        std::vector<int> values;
        const int mode = static_cast<int>(rng.uniform(0, 2));
        if (mode == 0) {
            values = rng.ints(n, 0, 6);
            std::sort(values.begin(), values.end());
        } else if (mode == 1) {
            values = rng.ints(n, 0, 6);
            std::sort(values.rbegin(), values.rend());
        } else {
            values = rng.ints(n, 0, 3);
        }
        bool max_ok = true, min_ok = true;
        for (std::size_t i = 1; i < n; ++i) {
            max_ok = max_ok && values[(i - 1) / 2] >= values[i];
            min_ok = min_ok && values[(i - 1) / 2] <= values[i];
        }
        const HeapKind expected =
            max_ok && min_ok ? HeapKind::Both : max_ok ? HeapKind::Max : min_ok ? HeapKind::Min : HeapKind::Neither;
        CHECK(classify_heap(values) == expected);
    }
}

TEST(2, "a min-heap needs no swaps") {
    std::vector<int> values{1, 2, 3, 4, 5};
    CHECK(min_heapify_trace(std::span<int>(values)).empty());
    CHECK_EQ(values, std::vector<int>{1, 2, 3, 4, 5});
}

TEST(2, "empty and single-element inputs") {
    std::vector<int> none;
    CHECK(min_heapify_trace(std::span<int>(none)).empty());
    std::vector<int> one{3};
    CHECK(min_heapify_trace(std::span<int>(one)).empty());
}

TEST(2, "a reversed array becomes a min-heap through valid swaps") {
    check_trace({7, 6, 5, 4, 3, 2, 1});
}

TEST(2, "duplicates do not cause needless swaps") {
    std::vector<int> values(32, 5);
    CHECK(min_heapify_trace(std::span<int>(values)).empty());
    check_trace({2, 2, 1, 3, 1});
}

TEST(2, "random arrays produce replayable traces") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 80));
        check_trace(rng.ints(n, -30, 30));
    }
}

TEST(3, "finds the minimum among the leaves") {
    const std::vector<int> heap{9, 8, 7, 3, 5, 1, 4};
    CHECK_EQ(min_of_max_heap<int>(heap), 1);
    const std::vector<int> single{4};
    CHECK_EQ(min_of_max_heap<int>(single), 4);
}

TEST(3, "an empty heap throws") {
    const std::vector<int> none;
    CHECK_THROWS_AS(min_of_max_heap<int>(none), std::invalid_argument);
}

TEST(3, "minimum finding uses at most half the elements") {
    Rng rng;
    for (std::size_t n : {1u, 2u, 3u, 7u, 8u, 1023u, 1024u, 5000u}) {
        INFO("n=", n);
        std::vector<Counted<int>> heap;
        for (std::size_t i = 0; i < n; ++i) {
            heap.emplace_back(static_cast<int>(rng.uniform(0, 1'000'000)));
        }
        std::make_heap(heap.begin(), heap.end());
        int expected = heap[0].value();
        for (const auto& value : heap) {
            expected = std::min(expected, value.value());
        }
        Counted<int>::comparisons() = 0;
        const auto& answer = min_of_max_heap<Counted<int>>(heap);
        CHECK_LE(Counted<int>::comparisons(), (n + 1) / 2);
        CHECK_EQ(answer.value(), expected);
    }
}

TEST(3, "converting a max-heap gives a min-heap with the same elements") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 200));
        auto values = rng.ints(n, -50, 50);
        std::make_heap(values.begin(), values.end());
        auto sorted = values;
        std::sort(sorted.begin(), sorted.end());
        max_heap_to_min_heap(std::span<int>(values));
        CHECK(is_min_heap(values));
        auto after = values;
        std::sort(after.begin(), after.end());
        CHECK_EQ(after, sorted);
    }
}

TEST(3, "conversion is linear in comparisons") {
    Rng rng;
    const std::size_t n = 100'000;
    std::vector<Counted<int>> heap;
    for (std::size_t i = 0; i < n; ++i) {
        heap.emplace_back(static_cast<int>(rng.uniform(0, 1'000'000'000)));
    }
    std::make_heap(heap.begin(), heap.end());
    Counted<int>::comparisons() = 0;
    max_heap_to_min_heap(std::span<Counted<int>>(heap));
    CHECK_LE(Counted<int>::comparisons(), 3 * n);
    for (std::size_t i = 1; i < n; ++i) {
        REQUIRE(heap[(i - 1) / 2].value() <= heap[i].value());
    }
}
