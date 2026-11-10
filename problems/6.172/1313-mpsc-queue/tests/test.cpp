#include <algorithm>
#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "08_dsched.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Stopwatch;
namespace dsched = practice::dsched;

namespace {

using Value = std::optional<std::uint64_t>;
using SchedQueue = MpscQueue<dsched::Atomic>;

std::vector<std::uint64_t> drain(SchedQueue& queue) {
    std::vector<std::uint64_t> values;
    while (values.size() < 32) {
        const auto value = queue.dequeue();
        if (!value) {
            break;
        }
        values.push_back(*value);
    }
    return values;
}

bool exactly_once(std::vector<std::uint64_t> seen, std::vector<std::uint64_t> expected) {
    std::sort(seen.begin(), seen.end());
    std::sort(expected.begin(), expected.end());
    return seen == expected;
}

const dsched::Options kBounded{.max_runs = 4000, .max_preemptions = 2, .max_steps = 3000};

long cas_operations = 0;

// std::atomic that counts compare-and-swap calls.
template <typename T>
class CountingAtomic : public std::atomic<T> {
public:
    using std::atomic<T>::atomic;

    bool compare_exchange_strong(T& expected, T desired, std::memory_order success,
                                 std::memory_order failure) {
        ++cas_operations;
        return std::atomic<T>::compare_exchange_strong(expected, desired, success, failure);
    }
    bool compare_exchange_strong(T& expected, T desired,
                                 std::memory_order order = std::memory_order_seq_cst) {
        ++cas_operations;
        return std::atomic<T>::compare_exchange_strong(expected, desired, order);
    }
    bool compare_exchange_weak(T& expected, T desired, std::memory_order success,
                               std::memory_order failure) {
        ++cas_operations;
        return std::atomic<T>::compare_exchange_weak(expected, desired, success, failure);
    }
    bool compare_exchange_weak(T& expected, T desired,
                               std::memory_order order = std::memory_order_seq_cst) {
        ++cas_operations;
        return std::atomic<T>::compare_exchange_weak(expected, desired, order);
    }
};

}  // namespace

TEST(1, "values come out in the order they went in") {
    MpscQueue<> queue(8);
    CHECK(queue.enqueue(1));
    CHECK(queue.enqueue(2));
    CHECK_EQ(queue.dequeue(), Value(1));
    CHECK(queue.enqueue(3));
    CHECK_EQ(queue.dequeue(), Value(2));
    CHECK_EQ(queue.dequeue(), Value(3));
    CHECK(!queue.dequeue().has_value());
}

TEST(1, "enqueue fails at capacity and recovers after a dequeue") {
    MpscQueue<> queue(2);
    CHECK(queue.enqueue(1));
    CHECK(queue.enqueue(2));
    CHECK(!queue.enqueue(3));
    CHECK_EQ(queue.dequeue(), Value(1));
    CHECK(queue.enqueue(3));
    CHECK_EQ(queue.dequeue(), Value(2));
    CHECK_EQ(queue.dequeue(), Value(3));
}

TEST(1, "nodes recycle indefinitely") {
    MpscQueue<> queue(2);
    for (std::uint64_t round = 1; round <= 5000; ++round) {
        REQUIRE(queue.enqueue(round));
        REQUIRE_EQ(queue.dequeue(), Value(round));
    }
}

TEST(1, "random operations match std::deque") {
    Rng rng;
    MpscQueue<> queue(32);
    std::deque<std::uint64_t> model;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        if (model.size() < 32 && (model.empty() || rng.coin())) {
            const auto value = static_cast<std::uint64_t>(rng.uniform(1, 1000));
            REQUIRE(queue.enqueue(value));
            model.push_back(value);
        } else {
            REQUIRE_EQ(queue.dequeue(), Value(model.front()));
            model.pop_front();
        }
    }
}

TEST(2, "two producers and the consumer lose nothing in any explored interleaving") {
    const auto report = dsched::explore_dfs(
        [] {
            auto queue = std::make_shared<SchedQueue>(4);
            auto got = std::make_shared<std::vector<std::uint64_t>>();
            dsched::Scenario scenario;
            scenario.threads = {[queue] { CHECK(queue->enqueue(1)); },
                                [queue] { CHECK(queue->enqueue(2)); },
                                [queue, got] {
                                    for (int i = 0; i < 2; ++i) {
                                        if (const auto value = queue->dequeue()) {
                                            got->push_back(*value);
                                        }
                                    }
                                }};
            scenario.check = [queue, got] {
                auto rest = drain(*queue);
                rest.insert(rest.end(), got->begin(), got->end());
                return exactly_once(rest, {1, 2});
            };
            return scenario;
        },
        kBounded);
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.clean());
}

TEST(2, "a freed dummy is never reused while the tail still points at it") {
    const auto report = dsched::explore_dfs(
        [] {
            // Capacity one: once the consumer frees the dummy, the second producer reuses it,
            // possibly while the first producer is stalled before moving the tail.
            auto queue = std::make_shared<SchedQueue>(1);
            auto got = std::make_shared<std::vector<std::uint64_t>>();
            auto first = std::make_shared<bool>(false);
            auto second = std::make_shared<bool>(false);
            dsched::Scenario scenario;
            scenario.threads = {[queue, first] { *first = queue->enqueue(1); },
                                [queue, second] { *second = queue->enqueue(2); },
                                [queue, got] {
                                    for (int i = 0; i < 2; ++i) {
                                        if (const auto value = queue->dequeue()) {
                                            got->push_back(*value);
                                        }
                                    }
                                }};
            scenario.check = [queue, got, first, second] {
                auto rest = drain(*queue);
                rest.insert(rest.end(), got->begin(), got->end());
                std::vector<std::uint64_t> expected;
                if (*first) {
                    expected.push_back(1);
                }
                if (*second) {
                    expected.push_back(2);
                }
                return exactly_once(rest, expected);
            };
            return scenario;
        },
        {.max_runs = 30'000, .max_preemptions = 2, .max_steps = 3000});
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(3, "the common case dequeue performs no compare-and-swap at all") {
    MpscQueue<CountingAtomic> queue(8);
    CHECK(queue.enqueue(1));
    CHECK(queue.enqueue(2));
    CHECK(queue.enqueue(3));
    cas_operations = 0;
    CHECK_EQ(queue.dequeue(), Value(1));
    CHECK_EQ(queue.dequeue(), Value(2));
    CHECK_EQ(cas_operations, 0);
}

TEST(3, "many real producers and one consumer lose nothing and keep per-producer order") {
    constexpr int kProducers = 4;
    constexpr int kPerProducer = 1500;
    const int total = kProducers * kPerProducer;
    MpscQueue<> queue(static_cast<std::size_t>(total) + 8);
    std::vector<std::uint64_t> got;
    got.reserve(static_cast<std::size_t>(total));

    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&queue, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                const std::uint64_t value =
                    static_cast<std::uint64_t>(p) * 1'000'000 + static_cast<std::uint64_t>(i) + 1;
                while (!queue.enqueue(value)) {
                    std::this_thread::yield();
                }
            }
        });
    }
    std::thread consumer([&] {
        Stopwatch stall;
        while (got.size() < static_cast<std::size_t>(total) && stall.elapsed_ms() < 20'000) {
            if (const auto value = queue.dequeue()) {
                got.push_back(*value);
                stall = Stopwatch{};
            }
        }
    });
    for (auto& producer : producers) {
        producer.join();
    }
    consumer.join();

    CHECK_EQ(got.size(), total);
    std::map<std::uint64_t, std::uint64_t> last;
    for (const std::uint64_t value : got) {
        const std::uint64_t producer = value / 1'000'000;
        CHECK(!last.contains(producer) || last[producer] < value);
        last[producer] = value;
    }
}
