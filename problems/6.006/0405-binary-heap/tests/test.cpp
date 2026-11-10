#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <set>
#include <span>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;
using practice::Tracked;

namespace {

template <typename T, typename Compare = std::less<T>>
void check_heap(std::span<const T> items, Compare compare = Compare{}) {
    for (std::size_t i = 1; i < items.size(); ++i) {
        REQUIRE(!compare(items[(i - 1) / 2], items[i]));
    }
}

int ceil_lg(std::size_t n) {
    int bits = 0;
    while ((std::size_t{1} << bits) < n) {
        ++bits;
    }
    return bits;
}

}  // namespace

TEST(1, "a new heap is empty") {
    BinaryHeap<int> heap;
    CHECK(heap.empty());
    CHECK_EQ(heap.size(), 0);
    CHECK_THROWS_AS(heap.top(), std::out_of_range);
    CHECK_THROWS_AS(heap.pop(), std::out_of_range);
}

TEST(1, "push keeps the maximum on top") {
    BinaryHeap<int> heap;
    heap.push(3);
    CHECK_EQ(heap.top(), 3);
    heap.push(9);
    CHECK_EQ(heap.top(), 9);
    heap.push(1);
    CHECK_EQ(heap.top(), 9);
    CHECK_EQ(heap.size(), 3);
    check_heap<int>(heap.items());
}

TEST(1, "pop returns values in descending order") {
    BinaryHeap<int> heap;
    for (int value : {5, 1, 8, 3, 9, 2}) {
        heap.push(value);
    }
    std::vector<int> popped;
    while (!heap.empty()) {
        popped.push_back(heap.pop());
        check_heap<int>(heap.items());
    }
    CHECK_EQ(popped, std::vector<int>{9, 8, 5, 3, 2, 1});
}

TEST(1, "duplicates and a single element") {
    BinaryHeap<int> heap;
    heap.push(4);
    heap.push(4);
    heap.push(4);
    CHECK_EQ(heap.pop(), 4);
    CHECK_EQ(heap.pop(), 4);
    CHECK_EQ(heap.top(), 4);
    CHECK_EQ(heap.pop(), 4);
    CHECK(heap.empty());
}

TEST(1, "std::greater turns it into a min-heap") {
    BinaryHeap<int, std::greater<int>> heap;
    for (int value : {5, 1, 8}) {
        heap.push(value);
    }
    CHECK_EQ(heap.pop(), 1);
    CHECK_EQ(heap.pop(), 5);
    CHECK_EQ(heap.pop(), 8);
}

TEST(1, "move-only elements") {
    struct Greater {
        bool operator()(const std::unique_ptr<int>& a, const std::unique_ptr<int>& b) const { return *a < *b; }
    };
    BinaryHeap<std::unique_ptr<int>, Greater> heap;
    heap.push(std::make_unique<int>(1));
    heap.push(std::make_unique<int>(3));
    heap.push(std::make_unique<int>(2));
    CHECK_EQ(*heap.pop(), 3);
    CHECK_EQ(*heap.pop(), 2);
}

TEST(1, "elements are released") {
    const auto before = Tracked::live();
    {
        struct Less {
            bool operator()(const Tracked& a, const Tracked& b) const { return a.value() < b.value(); }
        };
        BinaryHeap<Tracked, Less> heap;
        for (int i = 0; i < 500; ++i) {
            heap.push(Tracked(i));
        }
        for (int i = 0; i < 200; ++i) {
            CHECK_EQ(heap.pop().value(), 499 - i);
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "random pushes and pops match std::multiset") {
    Rng rng;
    BinaryHeap<int> heap;
    std::multiset<int> model;
    for (int step = 0; step < 100'000; ++step) {
        INFO("step ", step);
        if (model.empty() || rng.coin(0.55)) {
            const int value = static_cast<int>(rng.uniform(0, 1000));
            heap.push(value);
            model.insert(value);
        } else {
            REQUIRE_EQ(heap.top(), *model.rbegin());
            REQUIRE_EQ(heap.pop(), *model.rbegin());
            model.erase(std::prev(model.end()));
        }
        REQUIRE_EQ(heap.size(), model.size());
        if (step % 256 == 0) {
            check_heap<int>(heap.items());
        }
    }
}

TEST(1, "push and pop use O(log n) comparisons") {
    Rng rng;
    const int n = 1 << 17;
    BinaryHeap<Counted<int>> heap;
    for (int i = 0; i < n; ++i) {
        heap.push(Counted<int>(static_cast<int>(rng.uniform(0, 1'000'000'000))));
    }
    Counted<int>::comparisons() = 0;
    const int rounds = 20'000;
    for (int i = 0; i < rounds; ++i) {
        heap.push(Counted<int>(static_cast<int>(rng.uniform(0, 1'000'000'000))));
        heap.pop();
    }
    CHECK_LE(Counted<int>::comparisons(), static_cast<std::uint64_t>(rounds * (3 * ceil_lg(n) + 8)));
}

PERF(1, "three million pushes and pops", 2500) {
    Rng rng;
    BinaryHeap<int> heap;
    for (int i = 0; i < 3'000'000; ++i) {
        heap.push(static_cast<int>(rng.uniform(0, 2'000'000'000)));
    }
    long long sum = 0;
    while (!heap.empty()) {
        sum += heap.pop();
    }
    CHECK_GT(sum, 0);
}

TEST(2, "building from a vector gives a heap") {
    BinaryHeap<int> heap(std::vector<int>{3, 1, 4, 1, 5, 9, 2, 6});
    CHECK_EQ(heap.size(), 8);
    check_heap<int>(heap.items());
    CHECK_EQ(heap.pop(), 9);
}

TEST(2, "empty and single-element builds") {
    BinaryHeap<int> none(std::vector<int>{});
    CHECK(none.empty());
    BinaryHeap<int> one(std::vector<int>{7});
    CHECK_EQ(one.top(), 7);
}

TEST(2, "builds use a custom comparator") {
    BinaryHeap<int, std::greater<int>> heap(std::vector<int>{5, 2, 8, 1}, std::greater<int>{});
    check_heap<int, std::greater<int>>(heap.items(), std::greater<int>{});
    CHECK_EQ(heap.pop(), 1);
}

TEST(2, "random arrays build and drain in order") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 500));
        auto values = rng.ints(n, 0, 50);
        BinaryHeap<int> heap(values);
        check_heap<int>(heap.items());
        std::sort(values.rbegin(), values.rend());
        std::vector<int> popped;
        while (!heap.empty()) {
            popped.push_back(heap.pop());
        }
        CHECK_EQ(popped, values);
    }
}

TEST(2, "build costs a linear number of comparisons") {
    Rng rng;
    const std::size_t n = 200'000;
    std::vector<Counted<int>> values;
    values.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        values.emplace_back(static_cast<int>(rng.uniform(0, 1'000'000'000)));
    }
    Counted<int>::comparisons() = 0;
    BinaryHeap<Counted<int>> heap(std::move(values));
    CHECK_LE(Counted<int>::comparisons(), static_cast<std::uint64_t>(3 * n));
    check_heap<Counted<int>>(heap.items());
}

TEST(2, "building from sorted input is also linear") {
    const std::size_t n = 100'000;
    std::vector<Counted<int>> ascending;
    for (std::size_t i = 0; i < n; ++i) {
        ascending.emplace_back(static_cast<int>(i));
    }
    Counted<int>::comparisons() = 0;
    BinaryHeap<Counted<int>> heap(std::move(ascending));
    CHECK_LE(Counted<int>::comparisons(), static_cast<std::uint64_t>(3 * n));
}

PERF(2, "building ten million elements", 2500) {
    Rng rng;
    std::vector<int> values(10'000'000);
    for (int& value : values) {
        value = static_cast<int>(rng.uniform(0, 2'000'000'000));
    }
    BinaryHeap<int> heap(std::move(values));
    CHECK_EQ(heap.size(), 10'000'000);
}

TEST(3, "sorts ascending in place") {
    std::vector<int> values{5, 2, 9, 1, 7};
    heapsort(values.begin(), values.end());
    CHECK_EQ(values, std::vector<int>{1, 2, 5, 7, 9});
}

TEST(3, "empty, single and already sorted inputs") {
    std::vector<int> none;
    heapsort(none.begin(), none.end());
    CHECK(none.empty());
    std::vector<int> one{1};
    heapsort(one.begin(), one.end());
    CHECK_EQ(one[0], 1);
    std::vector<int> sorted{1, 2, 3, 4};
    heapsort(sorted.begin(), sorted.end());
    CHECK_EQ(sorted, std::vector<int>{1, 2, 3, 4});
    std::vector<int> reversed{4, 3, 2, 1};
    heapsort(reversed.begin(), reversed.end());
    CHECK_EQ(reversed, std::vector<int>{1, 2, 3, 4});
}

TEST(3, "a custom comparator flips the order") {
    std::vector<int> values{1, 3, 2};
    heapsort(values.begin(), values.end(), std::greater<int>{});
    CHECK_EQ(values, std::vector<int>{3, 2, 1});
}

TEST(3, "works on a raw array and a partial range") {
    int values[] = {5, 4, 3, 2, 1};
    heapsort(values + 1, values + 4);
    CHECK_EQ(std::vector<int>(values, values + 5), std::vector<int>{5, 2, 3, 4, 1});
}

TEST(3, "random arrays sort correctly") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 300));
        auto values = rng.ints(n, -20, 20);
        auto expected = values;
        std::sort(expected.begin(), expected.end());
        heapsort(values.begin(), values.end());
        CHECK_EQ(values, expected);
    }
}

TEST(3, "uses O(n log n) comparisons even on adversarial-looking data") {
    Rng rng;
    const std::size_t n = 100'000;
    std::vector<Counted<int>> values;
    for (std::size_t i = 0; i < n; ++i) {
        values.emplace_back(static_cast<int>(rng.uniform(0, 1'000'000)));
    }
    Counted<int>::comparisons() = 0;
    heapsort(values.begin(), values.end());
    CHECK_LE(Counted<int>::comparisons(), static_cast<std::uint64_t>(2 * n * static_cast<std::size_t>(ceil_lg(n)) + 3 * n));
    CHECK(std::is_sorted(values.begin(), values.end()));
}

TEST(3, "really works in place") {
    Rng rng;
    const std::size_t n = 2000;
    std::vector<Tracked> values;
    values.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        values.emplace_back(static_cast<int>(rng.uniform(0, 1'000'000)));
    }
    const auto before = Tracked::live();
    auto peak = before;
    heapsort(values.begin(), values.end(), [&](const Tracked& a, const Tracked& b) {
        peak = std::max(peak, Tracked::live());
        return a.value() < b.value();
    });
    CHECK_LE(peak, before + 4);
    CHECK_EQ(Tracked::live(), before);
    CHECK(std::is_sorted(values.begin(), values.end(),
                         [](const Tracked& a, const Tracked& b) { return a.value() < b.value(); }));
}

PERF(3, "two million integers", 4000) {
    Rng rng;
    std::vector<int> values(2'000'000);
    for (int& value : values) {
        value = static_cast<int>(rng.uniform(0, 2'000'000'000));
    }
    heapsort(values.begin(), values.end());
    CHECK(std::is_sorted(values.begin(), values.end()));
}

TEST(4, "emits events once more than the allowed lateness are buffered") {
    ReorderBuffer<int> buffer(2);
    CHECK_EQ(buffer.push(3), std::optional<int>());
    CHECK_EQ(buffer.push(1), std::optional<int>());
    CHECK_EQ(buffer.push(2), std::optional<int>(1));
    CHECK_EQ(buffer.push(5), std::optional<int>(2));
    CHECK_EQ(buffer.buffered(), 2);
    CHECK_EQ(buffer.drain(), std::vector<int>{3, 5});
    CHECK_EQ(buffer.buffered(), 0);
}

TEST(4, "zero lateness passes everything straight through") {
    ReorderBuffer<int> buffer(0);
    CHECK_EQ(buffer.push(1), std::optional<int>(1));
    CHECK_EQ(buffer.push(2), std::optional<int>(2));
    CHECK(buffer.drain().empty());
}

TEST(4, "draining an empty buffer gives nothing") {
    ReorderBuffer<int> buffer(5);
    CHECK(buffer.drain().empty());
}

TEST(4, "a custom order and a lateness larger than the stream") {
    ReorderBuffer<int, std::greater<int>> buffer(10, std::greater<int>{});
    for (int value : {1, 3, 2}) {
        CHECK(!buffer.push(value).has_value());
    }
    CHECK_EQ(buffer.drain(), std::vector<int>{3, 2, 1});
}

TEST(4, "random k-proximate streams come out fully sorted") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 2000));
        const auto k = static_cast<std::size_t>(rng.uniform(0, 30));
        std::vector<int> sorted = rng.ints(n, 0, 100'000);
        std::sort(sorted.begin(), sorted.end());
        // Shuffling inside blocks of k + 1 moves every value at most k places.
        std::vector<int> stream = sorted;
        for (std::size_t start = 0; start < n; start += k + 1) {
            const auto end = std::min(n, start + k + 1);
            std::span<int> block(stream.data() + start, end - start);
            rng.shuffle(block);
        }
        ReorderBuffer<int> buffer(k);
        std::vector<int> output;
        for (int value : stream) {
            if (auto out = buffer.push(value)) {
                output.push_back(*out);
            }
            REQUIRE(buffer.buffered() <= k);
        }
        for (int value : buffer.drain()) {
            output.push_back(value);
        }
        CHECK_EQ(output, sorted);
    }
}

TEST(4, "comparisons per push depend on the lateness, not the stream length") {
    Rng rng;
    const std::size_t n = 500'000;
    const std::size_t k = 3;
    std::vector<Counted<int>> stream;
    for (std::size_t i = 0; i < n; ++i) {
        stream.emplace_back(static_cast<int>(rng.uniform(0, 1'000'000)));
    }
    Counted<int>::comparisons() = 0;
    ReorderBuffer<Counted<int>> buffer(k);
    for (auto& value : stream) {
        buffer.push(std::move(value));
    }
    buffer.drain();
    CHECK_LE(Counted<int>::comparisons(), static_cast<std::uint64_t>(n * (3 * static_cast<std::size_t>(ceil_lg(k + 2)) + 6)));
}

PERF(4, "twenty million small-window events", 3000) {
    ReorderBuffer<int> buffer(16);
    Rng rng;
    long long sum = 0;
    int base = 0;
    for (int i = 0; i < 20'000'000; ++i) {
        if (auto out = buffer.push(base + static_cast<int>(rng.uniform(0, 16)))) {
            sum += *out;
        }
        ++base;
    }
    for (int value : buffer.drain()) {
        sum += value;
    }
    CHECK_GT(sum, 0);
}
