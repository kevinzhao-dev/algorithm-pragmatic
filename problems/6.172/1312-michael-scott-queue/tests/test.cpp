#include <algorithm>
#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
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
using SchedQueue = MsQueue<dsched::Atomic>;

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

}  // namespace

TEST(1, "values come out in the order they went in") {
    MsQueue<> queue(8);
    CHECK(queue.enqueue(1));
    CHECK(queue.enqueue(2));
    CHECK(queue.enqueue(3));
    CHECK_EQ(queue.dequeue(), Value(1));
    CHECK(queue.enqueue(4));
    CHECK_EQ(queue.dequeue(), Value(2));
    CHECK_EQ(queue.dequeue(), Value(3));
    CHECK_EQ(queue.dequeue(), Value(4));
    CHECK(!queue.dequeue().has_value());
}

TEST(1, "an empty queue stays empty") {
    MsQueue<> queue(4);
    CHECK(!queue.dequeue().has_value());
    CHECK(!queue.dequeue().has_value());
}

TEST(1, "enqueue fails at capacity and recovers") {
    MsQueue<> queue(2);
    CHECK(queue.enqueue(1));
    CHECK(queue.enqueue(2));
    CHECK(!queue.enqueue(3));
    CHECK_EQ(queue.dequeue(), Value(1));
    CHECK(queue.enqueue(3));
    CHECK_EQ(queue.dequeue(), Value(2));
    CHECK_EQ(queue.dequeue(), Value(3));
}

TEST(1, "nodes recycle indefinitely") {
    MsQueue<> queue(2);
    for (std::uint64_t round = 1; round <= 5000; ++round) {
        REQUIRE(queue.enqueue(round));
        REQUIRE_EQ(queue.dequeue(), Value(round));
    }
}

TEST(1, "random operations match std::deque") {
    Rng rng;
    MsQueue<> queue(32);
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

TEST(2, "two concurrent enqueues both land") {
    const auto report = dsched::explore_dfs(
        [] {
            auto queue = std::make_shared<SchedQueue>(4);
            dsched::Scenario scenario;
            scenario.threads = {[queue] { CHECK(queue->enqueue(1)); },
                                [queue] { CHECK(queue->enqueue(2)); }};
            scenario.check = [queue] { return exactly_once(drain(*queue), {1, 2}); };
            return scenario;
        },
        kBounded);
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "a dequeue racing an enqueue sees nothing or the value") {
    const auto report = dsched::explore_dfs(
        [] {
            auto queue = std::make_shared<SchedQueue>(4);
            auto seen = std::make_shared<Value>();
            dsched::Scenario scenario;
            scenario.threads = {[queue] { queue->enqueue(5); },
                                [queue, seen] { *seen = queue->dequeue(); }};
            scenario.check = [queue, seen] {
                auto rest = drain(*queue);
                if (seen->has_value()) {
                    rest.push_back(**seen);
                }
                return exactly_once(rest, {5});
            };
            return scenario;
        },
        kBounded);
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "a stalled enqueuer never blocks the others") {
    const auto report = dsched::explore_dfs(
        [] {
            auto queue = std::make_shared<SchedQueue>(4);
            auto got = std::make_shared<Value>();
            dsched::Scenario scenario;
            scenario.threads = {[queue] { CHECK(queue->enqueue(1)); },
                                [queue, got] {
                                    CHECK(queue->enqueue(2));
                                    *got = queue->dequeue();
                                }};
            scenario.check = [queue, got] {
                auto rest = drain(*queue);
                if (got->has_value()) {
                    rest.push_back(**got);
                }
                return exactly_once(rest, {1, 2});
            };
            return scenario;
        },
        kBounded);
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.clean());
}

TEST(2, "producers and a consumer agree on order in every explored interleaving") {
    const auto report = dsched::explore_dfs(
        [] {
            auto queue = std::make_shared<SchedQueue>(4);
            auto got = std::make_shared<std::vector<std::uint64_t>>();
            dsched::Scenario scenario;
            scenario.threads = {[queue] {
                                    CHECK(queue->enqueue(101));
                                    CHECK(queue->enqueue(102));
                                },
                                [queue, got] {
                                    for (int i = 0; i < 2; ++i) {
                                        if (const auto value = queue->dequeue()) {
                                            got->push_back(*value);
                                        }
                                    }
                                }};
            scenario.check = [queue, got] {
                const auto rest = drain(*queue);
                std::vector<std::uint64_t> all = *got;
                all.insert(all.end(), rest.begin(), rest.end());
                return std::is_sorted(all.begin(), all.end()) && exactly_once(all, {101, 102});
            };
            return scenario;
        },
        kBounded);
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.clean());
}

TEST(3, "a stale dequeue cannot pop through a recycled head") {
    const auto report = dsched::explore_dfs(
        [] {
            // Two values and a dummy use every node, so the operations below recycle them all.
            auto queue = std::make_shared<SchedQueue>(2);
            queue->enqueue(1);
            queue->enqueue(2);
            auto stale = std::make_shared<Value>();
            auto other = std::make_shared<std::vector<std::uint64_t>>();
            dsched::Scenario scenario;
            scenario.threads = {[queue, stale] { *stale = queue->dequeue(); },
                                [queue, other] {
                                    for (int i = 0; i < 2; ++i) {
                                        if (const auto value = queue->dequeue()) {
                                            other->push_back(*value);
                                        }
                                    }
                                    CHECK(queue->enqueue(3));
                                    if (const auto value = queue->dequeue()) {
                                        other->push_back(*value);
                                    }
                                }};
            scenario.check = [queue, stale, other] {
                auto rest = drain(*queue);
                rest.insert(rest.end(), other->begin(), other->end());
                if (stale->has_value()) {
                    rest.push_back(**stale);
                }
                return exactly_once(rest, {1, 2, 3});
            };
            return scenario;
        },
        kBounded);
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.clean());
}

TEST(4, "real producers and consumers lose nothing and keep per-producer order") {
    constexpr int kProducers = 2;
    constexpr int kConsumers = 2;
    constexpr int kPerProducer = 3000;
    const int total = kProducers * kPerProducer;
    MsQueue<> queue(static_cast<std::size_t>(total) + 8);
    std::atomic<int> consumed{0};
    std::mutex results_mutex;
    std::vector<std::vector<std::uint64_t>> per_consumer(kConsumers);

    std::vector<std::thread> threads;
    for (int p = 0; p < kProducers; ++p) {
        threads.emplace_back([&queue, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                const std::uint64_t value =
                    static_cast<std::uint64_t>(p) * 1'000'000 + static_cast<std::uint64_t>(i) + 1;
                while (!queue.enqueue(value)) {
                    std::this_thread::yield();
                }
            }
        });
    }
    for (int c = 0; c < kConsumers; ++c) {
        threads.emplace_back([&, c] {
            std::vector<std::uint64_t> mine;
            Stopwatch stall;
            while (consumed.load() < total && stall.elapsed_ms() < 20'000) {
                if (const auto value = queue.dequeue()) {
                    mine.push_back(*value);
                    consumed.fetch_add(1);
                    stall = Stopwatch{};
                }
            }
            const std::lock_guard lock(results_mutex);
            per_consumer[static_cast<std::size_t>(c)] = std::move(mine);
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    std::vector<std::uint64_t> all;
    for (const auto& mine : per_consumer) {
        std::map<std::uint64_t, std::uint64_t> last;
        for (const std::uint64_t value : mine) {
            const std::uint64_t producer = value / 1'000'000;
            CHECK(!last.contains(producer) || last[producer] < value);
            last[producer] = value;
        }
        all.insert(all.end(), mine.begin(), mine.end());
    }
    CHECK_EQ(all.size(), total);
    std::sort(all.begin(), all.end());
    CHECK(std::adjacent_find(all.begin(), all.end()) == all.end());
}
