#include <cstddef>
#include <cstdint>
#include <deque>
#include <stdexcept>
#include <vector>

#include "00_counters.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;
using practice6006::Probe;

namespace {

template <typename T>
std::vector<int> values_of(const RingDeque<T>& deque) {
    std::vector<int> out;
    for (std::size_t i = 0; i < deque.size(); ++i) {
        if constexpr (requires(const T& item) { item.value(); }) {
            out.push_back(deque.get_at(i).value());
        } else {
            out.push_back(deque.get_at(i));
        }
    }
    return out;
}

template <typename T>
void check_capacity(const RingDeque<T>& deque) {
    CHECK_LE(deque.size(), deque.capacity());
    CHECK_LE(deque.capacity(), 4 * deque.size() + 16);
}

}  // namespace

TEST(1, "a new deque is empty") {
    RingDeque<int> deque;
    CHECK(deque.empty());
    CHECK_EQ(deque.size(), 0);
}

TEST(1, "operating on an empty deque throws") {
    RingDeque<int> deque;
    CHECK_THROWS_AS(deque.delete_first(), std::out_of_range);
    CHECK_THROWS_AS(deque.delete_last(), std::out_of_range);
    CHECK_THROWS_AS(deque.get_at(0), std::out_of_range);
    CHECK_THROWS_AS(deque.set_at(0, 1), std::out_of_range);
}

TEST(1, "first in first out through both ends") {
    RingDeque<int> deque;
    deque.insert_last(1);
    deque.insert_last(2);
    deque.insert_first(0);
    deque.insert_first(-1);
    CHECK_EQ(values_of(deque), std::vector<int>{-1, 0, 1, 2});
    CHECK_EQ(deque.delete_first(), -1);
    CHECK_EQ(deque.delete_last(), 2);
    CHECK_EQ(values_of(deque), std::vector<int>{0, 1});
    CHECK_EQ(deque.size(), 2);
}

TEST(1, "indexed access counts from the front") {
    RingDeque<int> deque;
    for (int i = 0; i < 5; ++i) {
        deque.insert_first(i);
    }
    CHECK_EQ(deque.get_at(0), 4);
    CHECK_EQ(deque.get_at(4), 0);
    deque.set_at(2, 100);
    CHECK_EQ(values_of(deque), std::vector<int>{4, 3, 100, 1, 0});
    CHECK_THROWS_AS(deque.get_at(5), std::out_of_range);
}

TEST(1, "a stream through a small window reuses the same storage") {
    RingDeque<int> deque;
    for (int i = 0; i < 8; ++i) {
        deque.insert_last(i);
    }
    const std::size_t capacity_after_fill = deque.capacity();
    for (int i = 8; i < 100'000; ++i) {
        CHECK_EQ(deque.delete_first(), i - 8);
        deque.insert_last(i);
    }
    CHECK_EQ(deque.size(), 8);
    CHECK_EQ(deque.capacity(), capacity_after_fill);
    CHECK_EQ(deque.get_at(0), 100'000 - 8);
    CHECK_EQ(deque.get_at(7), 99'999);
}

TEST(1, "indexes survive wrap-around from the back") {
    RingDeque<int> deque;
    for (int i = 0; i < 4; ++i) {
        deque.insert_last(i);
    }
    // Rotate through the front many times with mixed end operations.
    for (int round = 0; round < 1'000; ++round) {
        deque.insert_last(deque.delete_first());
    }
    for (int round = 0; round < 1'003; ++round) {
        deque.insert_first(deque.delete_last());
    }
    CHECK_EQ(values_of(deque), std::vector<int>{1, 2, 3, 0});
}

TEST(1, "elements are released") {
    const auto before = Tracked::live();
    {
        RingDeque<Tracked> deque;
        for (int i = 0; i < 100; ++i) {
            deque.insert_last(Tracked(i));
            deque.insert_first(Tracked(-i));
        }
        CHECK_EQ(deque.delete_first().value(), -99);
        CHECK_EQ(deque.delete_last().value(), 99);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "random operations match std::deque") {
    Rng rng;
    RingDeque<int> deque;
    std::deque<int> model;
    for (int step = 0; step < 100'000; ++step) {
        INFO("step ", step);
        const auto op = rng.uniform(0, 5);
        const int value = static_cast<int>(rng.uniform(0, 1'000'000));
        if (op == 0) {
            deque.insert_first(value);
            model.push_front(value);
        } else if (op == 1) {
            deque.insert_last(value);
            model.push_back(value);
        } else if (op == 2 && !model.empty()) {
            REQUIRE_EQ(deque.delete_first(), model.front());
            model.pop_front();
        } else if (op == 3 && !model.empty()) {
            REQUIRE_EQ(deque.delete_last(), model.back());
            model.pop_back();
        } else if (op == 4 && !model.empty()) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            REQUIRE_EQ(deque.get_at(i), model[i]);
        } else if (op == 5 && !model.empty()) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            deque.set_at(i, value);
            model[i] = value;
        }
        REQUIRE_EQ(deque.size(), model.size());
    }
    CHECK_EQ(values_of(deque), std::vector<int>(model.begin(), model.end()));
}

TEST(2, "capacity stays proportional to size") {
    Rng rng;
    RingDeque<int> deque;
    for (int step = 0; step < 100'000; ++step) {
        const bool grow = (step / 8'000) % 2 == 0 ? rng.coin(0.7) : rng.coin(0.3);
        if (grow || deque.empty()) {
            if (rng.coin()) {
                deque.insert_first(step);
            } else {
                deque.insert_last(step);
            }
        } else if (rng.coin()) {
            deque.delete_first();
        } else {
            deque.delete_last();
        }
        INFO("step ", step);
        check_capacity(deque);
    }
}

TEST(2, "element writes are amortized constant in any mix of ends") {
    using Item = Probe<int>;
    Rng rng;
    RingDeque<Item> deque;
    Item::reset();
    std::size_t operations = 0;
    for (int phase = 0; phase < 200; ++phase) {
        const auto target = static_cast<std::size_t>(rng.uniform(0, 2'000));
        while (deque.size() < target) {
            if (rng.coin()) {
                deque.insert_first(Item(1));
            } else {
                deque.insert_last(Item(1));
            }
            ++operations;
        }
        while (deque.size() > target) {
            if (rng.coin()) {
                deque.delete_first();
            } else {
                deque.delete_last();
            }
            ++operations;
        }
    }
    CHECK_LE(Item::writes(), 9 * operations + 64);
}

TEST(2, "alternating ends at a growth boundary does not thrash") {
    using Item = Probe<int>;
    RingDeque<Item> deque;
    while (deque.size() < 1'000 || deque.size() != deque.capacity()) {
        deque.insert_last(Item(0));
    }
    Item::reset();
    const std::size_t rounds = 20'000;
    for (std::size_t round = 0; round < rounds; ++round) {
        deque.insert_first(Item(1));
        deque.delete_first();
        deque.insert_last(Item(2));
        deque.delete_last();
    }
    CHECK_LE(Item::writes(), 12 * rounds + 3 * deque.capacity());
}

PERF(2, "tens of millions of operations at both ends", 3000) {
    RingDeque<std::int64_t> deque;
    for (std::int64_t i = 0; i < 10'000'000; ++i) {
        deque.insert_last(i);
        deque.insert_first(i);
    }
    std::int64_t sum = 0;
    for (std::int64_t i = 0; i < 10'000'000; ++i) {
        sum += deque.delete_first();
        sum += deque.delete_last();
    }
    CHECK_GT(sum, 0);
}

TEST(3, "the limiter allows up to limit requests per window") {
    SlidingWindowLimiter limiter(2, 10);
    CHECK(limiter.allow(0));
    CHECK(limiter.allow(1));
    CHECK(!limiter.allow(2));
    CHECK_EQ(limiter.active(2), 2);
}

TEST(3, "the window slides strictly") {
    SlidingWindowLimiter limiter(1, 10);
    CHECK(limiter.allow(0));
    CHECK(!limiter.allow(9));   // (−1, 9] still contains the request at 0
    CHECK(limiter.allow(10));   // (0, 10] does not contain it any more
}

TEST(3, "rejected requests do not count against the window") {
    SlidingWindowLimiter limiter(1, 5);
    CHECK(limiter.allow(0));
    CHECK(!limiter.allow(1));
    CHECK(!limiter.allow(4));
    // Only the accepted request at time 0 matters: at time 5, (0, 5] is empty.
    CHECK(limiter.allow(5));
}

TEST(3, "active expires old requests") {
    SlidingWindowLimiter limiter(5, 3);
    limiter.allow(1);
    limiter.allow(2);
    limiter.allow(4);
    CHECK_EQ(limiter.active(4), 2);  // (1, 4] holds requests at 2 and 4
    CHECK_EQ(limiter.active(5), 1);  // (2, 5] holds only the request at 4
    CHECK_EQ(limiter.active(10), 0);
}

TEST(3, "bursts at the same timestamp respect the limit") {
    SlidingWindowLimiter limiter(3, 100);
    CHECK(limiter.allow(5));
    CHECK(limiter.allow(5));
    CHECK(limiter.allow(5));
    CHECK(!limiter.allow(5));
    CHECK_EQ(limiter.active(5), 3);
}

TEST(3, "invalid configuration or decreasing time throws invalid_argument") {
    CHECK_THROWS_AS(SlidingWindowLimiter(0, 1), std::invalid_argument);
    CHECK_THROWS_AS(SlidingWindowLimiter(1, 0), std::invalid_argument);
    SlidingWindowLimiter limiter(1, 5);
    CHECK(limiter.allow(10));
    CHECK_THROWS_AS(limiter.allow(9), std::invalid_argument);
    CHECK_THROWS_AS(limiter.active(9), std::invalid_argument);
}

TEST(3, "random workload matches a brute-force log") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const auto limit = static_cast<std::size_t>(rng.uniform(1, 6));
        const std::int64_t window = rng.uniform(1, 30);
        SlidingWindowLimiter limiter(limit, window);
        std::vector<std::int64_t> accepted;
        std::int64_t now = 0;
        for (int step = 0; step < 300; ++step) {
            now += rng.uniform(0, 3);
            const auto in_window = [&] {
                std::size_t count = 0;
                for (const std::int64_t t : accepted) {
                    count += t > now - window ? 1 : 0;
                }
                return count;
            };
            INFO("round ", round, " step ", step, " now=", now);
            if (rng.coin(0.2)) {
                REQUIRE_EQ(limiter.active(now), in_window());
            } else {
                const bool expected = in_window() < limit;
                REQUIRE_EQ(limiter.allow(now), expected);
                if (expected) {
                    accepted.push_back(now);
                }
            }
        }
    }
}

// With limit 500'000, scanning the recorded requests per call would be ruinous.
PERF(3, "allow is amortized constant regardless of the limit", 3000) {
    SlidingWindowLimiter limiter(500'000, 1'000'000);
    std::int64_t t = 0;
    std::size_t accepted = 0;
    for (int i = 0; i < 20'000'000; ++i) {
        t += i % 4 == 0 ? 1 : 0;
        accepted += limiter.allow(t) ? 1 : 0;
    }
    CHECK_GT(accepted, 0);
}
