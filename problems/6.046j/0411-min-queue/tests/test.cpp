#include <deque>
#include <memory>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;
using practice::Tracked;

namespace {

struct MoveOnly {
    explicit MoveOnly(int v) : value(std::make_unique<int>(v)) {}
    std::unique_ptr<int> value;
    friend bool operator<(const MoveOnly& a, const MoveOnly& b) { return *a.value < *b.value; }
};

}  // namespace

TEST(1, "a new queue is empty") {
    const MinQueue<int> queue;
    CHECK(queue.empty());
    CHECK_EQ(queue.size(), 0);
}

TEST(1, "operating on an empty queue throws") {
    MinQueue<int> queue;
    CHECK_THROWS_AS(queue.dequeue(), std::out_of_range);
    CHECK_THROWS_AS(queue.front(), std::out_of_range);
    CHECK_THROWS_AS(queue.find_min(), std::out_of_range);
}

TEST(1, "first in is first out") {
    MinQueue<int> queue;
    for (int value : {3, 1, 2}) {
        queue.enqueue(value);
    }
    CHECK_EQ(queue.size(), 3);
    CHECK_EQ(queue.front(), 3);
    CHECK_EQ(queue.dequeue(), 3);
    CHECK_EQ(queue.dequeue(), 1);
    CHECK_EQ(queue.dequeue(), 2);
    CHECK(queue.empty());
}

TEST(1, "the minimum is reported and updated as elements leave") {
    MinQueue<int> queue;
    queue.enqueue(5);
    CHECK_EQ(queue.find_min(), 5);
    queue.enqueue(3);
    CHECK_EQ(queue.find_min(), 3);
    queue.enqueue(4);
    CHECK_EQ(queue.find_min(), 3);
    queue.dequeue();
    CHECK_EQ(queue.find_min(), 3);
    queue.dequeue();
    CHECK_EQ(queue.find_min(), 4);
}

TEST(1, "equal values are tracked individually") {
    MinQueue<int> queue;
    queue.enqueue(1);
    queue.enqueue(1);
    queue.enqueue(1);
    CHECK_EQ(queue.dequeue(), 1);
    CHECK_EQ(queue.find_min(), 1);
    CHECK_EQ(queue.dequeue(), 1);
    CHECK_EQ(queue.find_min(), 1);
    CHECK_EQ(queue.dequeue(), 1);
    CHECK(queue.empty());
}

TEST(1, "a smaller value arriving later hides earlier larger ones") {
    MinQueue<int> queue;
    queue.enqueue(9);
    queue.enqueue(8);
    queue.enqueue(1);
    queue.dequeue();
    queue.dequeue();
    CHECK_EQ(queue.find_min(), 1);
}

TEST(1, "copying a queue copies its full state") {
    MinQueue<int> queue;
    queue.enqueue(2);
    queue.enqueue(1);
    MinQueue<int> copy = queue;
    copy.dequeue();
    copy.dequeue();
    CHECK_EQ(queue.size(), 2);
    CHECK_EQ(queue.find_min(), 1);
    CHECK(copy.empty());
}

TEST(1, "random operations match a scan of the live elements") {
    Rng rng;
    MinQueue<int> queue;
    std::deque<int> model;
    for (int step = 0; step < 30'000; ++step) {
        INFO("step ", step);
        if (model.empty() || rng.coin(0.55)) {
            const int value = static_cast<int>(rng.uniform(0, 30));
            queue.enqueue(value);
            model.push_back(value);
        } else {
            REQUIRE_EQ(queue.dequeue(), model.front());
            model.pop_front();
        }
        REQUIRE_EQ(queue.size(), model.size());
        if (!model.empty()) {
            REQUIRE_EQ(queue.front(), model.front());
            REQUIRE_EQ(queue.find_min(), *std::ranges::min_element(model));
        }
    }
}

TEST(2, "a descending run costs linear comparisons overall") {
    Counted<int>::comparisons() = 0;
    MinQueue<Counted<int>> queue;
    const int n = 200'000;
    for (int value = n; value > 0; --value) {
        queue.enqueue(Counted<int>(value));
    }
    for (int i = 0; i < n; ++i) {
        queue.dequeue();
    }
    CHECK_LE(Counted<int>::comparisons(), 3ULL * n);
}

TEST(2, "one huge drop after an ascending run is charged to the elements it removes") {
    Counted<int>::comparisons() = 0;
    MinQueue<Counted<int>> queue;
    const int n = 200'000;
    for (int value = 1; value <= n; ++value) {
        queue.enqueue(Counted<int>(value));
    }
    queue.enqueue(Counted<int>(0));
    CHECK_EQ(queue.find_min().value(), 0);
    CHECK_LE(Counted<int>::comparisons(), 3ULL * (n + 1));
}

TEST(2, "a random mixed workload stays amortized constant") {
    Rng rng;
    Counted<int>::comparisons() = 0;
    MinQueue<Counted<int>> queue;
    const std::uint64_t operations = 400'000;
    std::uint64_t in_queue = 0;
    for (std::uint64_t step = 0; step < operations; ++step) {
        if (in_queue == 0 || rng.coin(0.5)) {
            queue.enqueue(Counted<int>(static_cast<int>(rng.uniform(0, 1000))));
            ++in_queue;
        } else {
            queue.dequeue();
            --in_queue;
        }
        if (in_queue != 0) {
            queue.find_min();
        }
    }
    CHECK_LE(Counted<int>::comparisons(), 3 * operations);
}

TEST(2, "a custom comparator turns it into a max queue") {
    MinQueue<int, std::greater<int>> queue;
    for (int value : {4, 9, 2}) {
        queue.enqueue(value);
    }
    CHECK_EQ(queue.find_min(), 9);
    queue.dequeue();
    queue.dequeue();
    CHECK_EQ(queue.find_min(), 2);
}

TEST(2, "a stateful comparator is honored") {
    int calls = 0;
    const auto compare = [&calls](int a, int b) {
        ++calls;
        return a < b;
    };
    MinQueue<int, decltype(compare)> queue(compare);
    queue.enqueue(3);
    queue.enqueue(2);
    CHECK_EQ(queue.find_min(), 2);
    CHECK_GT(calls, 0);
}

TEST(2, "move-only elements are supported") {
    MinQueue<MoveOnly> queue;
    queue.enqueue(MoveOnly(5));
    queue.enqueue(MoveOnly(2));
    queue.enqueue(MoveOnly(8));
    CHECK_EQ(*queue.find_min().value, 2);
    CHECK_EQ(*queue.dequeue().value, 5);
    CHECK_EQ(*queue.front().value, 2);
    queue.dequeue();
    CHECK_EQ(*queue.find_min().value, 8);
}

TEST(2, "no element leaks") {
    const auto before = Tracked::live();
    {
        const auto less = [](const Tracked& a, const Tracked& b) { return a.value() < b.value(); };
        MinQueue<Tracked, decltype(less)> queue(less);
        for (int i = 0; i < 3000; ++i) {
            queue.enqueue(Tracked(i % 17));
            if (i % 3 == 0) {
                queue.dequeue();
            }
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

PERF(2, "twenty million queue operations", 2500) {
    Rng rng;
    MinQueue<int> queue;
    std::int64_t checksum = 0;
    for (int i = 0; i < 10'000'000; ++i) {
        queue.enqueue(i * 7 % 1000);
        checksum += queue.find_min();
        if (i % 2 == 0 && !queue.empty()) {
            checksum += queue.dequeue();
        }
    }
    CHECK_GT(checksum, 0);
}

TEST(3, "a non-positive window is rejected") {
    CHECK_THROWS_AS(PeakTracker(0), std::invalid_argument);
    CHECK_THROWS_AS(PeakTracker(-1), std::invalid_argument);
}

TEST(3, "an empty window has no peak") {
    PeakTracker tracker(10);
    CHECK(!tracker.peak(5).has_value());
}

TEST(3, "the window is half open on the old side") {
    PeakTracker tracker(10);
    tracker.record(5, 100);
    CHECK_EQ(tracker.peak(14), std::optional<std::int64_t>(100));
    CHECK(!tracker.peak(15).has_value());
}

TEST(3, "the peak tracks the largest live sample") {
    PeakTracker tracker(10);
    tracker.record(1, 50);
    tracker.record(3, 70);
    tracker.record(5, 20);
    CHECK_EQ(tracker.peak(5), std::optional<std::int64_t>(70));
    CHECK_EQ(tracker.peak(12), std::optional<std::int64_t>(70));
    CHECK_EQ(tracker.peak(13), std::optional<std::int64_t>(20));
}

TEST(3, "time going backwards is rejected") {
    PeakTracker tracker(10);
    tracker.record(5, 1);
    CHECK_THROWS_AS(tracker.record(4, 1), std::invalid_argument);
    CHECK_THROWS_AS(tracker.peak(3), std::invalid_argument);
}

TEST(3, "equal timestamps are fine") {
    PeakTracker tracker(1);
    tracker.record(7, 1);
    tracker.record(7, 3);
    tracker.record(7, 2);
    CHECK_EQ(tracker.peak(7), std::optional<std::int64_t>(3));
    CHECK(!tracker.peak(8).has_value());
}

TEST(3, "random sequences match a scan of the window") {
    Rng rng;
    for (int trial = 0; trial < 200; ++trial) {
        INFO("trial ", trial);
        const std::int64_t window = rng.uniform(1, 20);
        PeakTracker tracker(window);
        std::vector<std::pair<std::int64_t, std::int64_t>> samples;
        std::int64_t now = 0;
        for (int step = 0; step < 400; ++step) {
            now += rng.uniform(0, 3);
            if (rng.coin()) {
                const std::int64_t value = rng.uniform(-1000, 1000);
                tracker.record(now, value);
                samples.emplace_back(now, value);
            } else {
                std::optional<std::int64_t> best;
                for (const auto& [time, value] : samples) {
                    if (time > now - window && (!best || value > *best)) {
                        best = value;
                    }
                }
                REQUIRE_EQ(tracker.peak(now), best);
            }
        }
    }
}

PERF(3, "a long stream with a wide window", 2000) {
    PeakTracker tracker(100'000);
    std::int64_t sum = 0;
    for (std::int64_t time = 0; time < 4'000'000; ++time) {
        tracker.record(time, (time * 37) % 1009);
        sum += tracker.peak(time).value();
    }
    CHECK_GT(sum, 0);
}
