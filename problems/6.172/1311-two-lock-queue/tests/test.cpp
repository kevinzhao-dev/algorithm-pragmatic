#include <algorithm>
#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "08_dsched.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Stopwatch;
using practice::Tracked;
namespace dsched = practice::dsched;

namespace {

using Value = std::optional<std::uint64_t>;
template <typename T>
using SchedTwoLock = TwoLockQueue<T, dsched::Mutex, dsched::Atomic>;
template <typename T>
using SchedSingle = SingleEnqueuerQueue<T, dsched::Mutex, dsched::Atomic>;

// A dsched mutex that also measures how many locks are held at once and how often it is taken.
struct LockStats {
    int held = 0;
    int max_held = 0;
    long acquisitions = 0;
};

LockStats* active_stats = nullptr;

class TrackingMutex {
public:
    void lock() {
        inner_.lock();
        ++active_stats->acquisitions;
        active_stats->max_held = std::max(active_stats->max_held, ++active_stats->held);
    }
    void unlock() {
        --active_stats->held;
        inner_.unlock();
    }

private:
    dsched::Mutex inner_;
};

template <typename Queue>
std::vector<std::uint64_t> drain(Queue& queue) {
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

// Values encode (producer, sequence); each producer's values must appear in order.
bool producers_in_order(const std::vector<std::uint64_t>& seen) {
    std::map<std::uint64_t, std::uint64_t> last;
    for (const std::uint64_t value : seen) {
        const std::uint64_t producer = value / 100;
        const std::uint64_t sequence = value % 100;
        if (last.contains(producer) && last[producer] >= sequence) {
            return false;
        }
        last[producer] = sequence;
    }
    return true;
}

template <template <typename> class SchedQueue>
void explore_producers_and_consumer() {
    const auto report = dsched::explore_dfs(
        [] {
            auto queue = std::make_shared<SchedQueue<std::uint64_t>>(4);
            auto got = std::make_shared<std::vector<std::uint64_t>>();
            dsched::Scenario scenario;
            for (std::uint64_t p = 1; p <= 2; ++p) {
                scenario.threads.push_back([queue, p] {
                    CHECK(queue->enqueue(p * 100 + 1));
                    CHECK(queue->enqueue(p * 100 + 2));
                });
            }
            scenario.threads.push_back([queue, got] {
                for (int i = 0; i < 2; ++i) {
                    if (const auto value = queue->dequeue()) {
                        got->push_back(*value);
                    }
                }
            });
            scenario.check = [queue, got] {
                auto rest = drain(*queue);
                std::vector<std::uint64_t> all = *got;
                all.insert(all.end(), rest.begin(), rest.end());
                return producers_in_order(all) && exactly_once(all, {101, 102, 201, 202});
            };
            return scenario;
        },
        {.max_preemptions = 2, .max_runs = 3000});
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.clean());
}

// Real producers and consumers; consumers give up when no progress is made for a long time.
template <typename Queue>
void stress_real_threads(int producers, int consumers, int per_producer) {
    Queue queue(static_cast<std::size_t>(producers * per_producer) + 8);
    const int total = producers * per_producer;
    std::atomic<int> consumed{0};
    std::mutex results_mutex;
    std::vector<std::vector<std::string>> per_consumer(static_cast<std::size_t>(consumers));

    std::vector<std::thread> threads;
    for (int p = 0; p < producers; ++p) {
        threads.emplace_back([&queue, p, per_producer] {
            for (int i = 0; i < per_producer; ++i) {
                while (!queue.enqueue(std::to_string(p) + ":" + std::to_string(i))) {
                    std::this_thread::yield();
                }
            }
        });
    }
    for (int c = 0; c < consumers; ++c) {
        threads.emplace_back([&, c] {
            std::vector<std::string> mine;
            Stopwatch stall;
            while (consumed.load() < total && stall.elapsed_ms() < 20'000) {
                if (auto value = queue.dequeue()) {
                    mine.push_back(std::move(*value));
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

    std::vector<std::string> all;
    for (const auto& mine : per_consumer) {
        // One consumer sees each producer's values in increasing sequence order.
        std::map<int, int> last;
        for (const std::string& item : mine) {
            const auto colon = item.find(':');
            const int producer = std::stoi(item.substr(0, colon));
            const int sequence = std::stoi(item.substr(colon + 1));
            CHECK(!last.contains(producer) || last[producer] < sequence);
            last[producer] = sequence;
        }
        all.insert(all.end(), mine.begin(), mine.end());
    }
    CHECK_EQ(all.size(), total);
    std::sort(all.begin(), all.end());
    CHECK(std::adjacent_find(all.begin(), all.end()) == all.end());
}

}  // namespace

TEST(1, "values come out in the order they went in") {
    TwoLockQueue<int> queue(8);
    CHECK(queue.enqueue(1));
    CHECK(queue.enqueue(2));
    CHECK(queue.enqueue(3));
    CHECK_EQ(queue.dequeue(), std::optional<int>(1));
    CHECK_EQ(queue.dequeue(), std::optional<int>(2));
    CHECK(queue.enqueue(4));
    CHECK_EQ(queue.dequeue(), std::optional<int>(3));
    CHECK_EQ(queue.dequeue(), std::optional<int>(4));
}

TEST(1, "dequeue on an empty queue returns nothing, repeatedly") {
    TwoLockQueue<int> queue(2);
    CHECK(!queue.dequeue().has_value());
    queue.enqueue(1);
    CHECK_EQ(queue.dequeue(), std::optional<int>(1));
    CHECK(!queue.dequeue().has_value());
    CHECK(!queue.dequeue().has_value());
}

TEST(1, "enqueue fails when the pool is exhausted and recovers after a dequeue") {
    TwoLockQueue<int> queue(2);
    CHECK(queue.enqueue(1));
    CHECK(queue.enqueue(2));
    CHECK(!queue.enqueue(3));
    CHECK_EQ(queue.dequeue(), std::optional<int>(1));
    CHECK(queue.enqueue(3));
    CHECK_EQ(queue.dequeue(), std::optional<int>(2));
    CHECK_EQ(queue.dequeue(), std::optional<int>(3));
}

TEST(1, "nodes are recycled so the capacity holds over many rounds") {
    TwoLockQueue<int> queue(3);
    for (int round = 0; round < 5000; ++round) {
        REQUIRE(queue.enqueue(round));
        REQUIRE(queue.enqueue(round + 1));
        REQUIRE_EQ(queue.dequeue(), std::optional<int>(round));
        REQUIRE_EQ(queue.dequeue(), std::optional<int>(round + 1));
    }
}

TEST(1, "a move-only value type works") {
    TwoLockQueue<std::unique_ptr<int>> queue(2);
    CHECK(queue.enqueue(std::make_unique<int>(7)));
    const auto value = queue.dequeue();
    REQUIRE(value.has_value());
    CHECK_EQ(**value, 7);
}

TEST(1, "element lifetimes are managed by the queue") {
    const auto before = Tracked::live();
    {
        TwoLockQueue<Tracked> queue(4);
        queue.enqueue(Tracked(1));
        queue.enqueue(Tracked(2));
        const auto value = queue.dequeue();
        CHECK_EQ(value->value(), 1);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "random operations match std::deque") {
    Rng rng;
    TwoLockQueue<int> queue(32);
    std::deque<int> model;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        if (model.size() < 32 && (model.empty() || rng.coin())) {
            const int value = static_cast<int>(rng.uniform(0, 1000));
            REQUIRE(queue.enqueue(value));
            model.push_back(value);
        } else {
            REQUIRE_EQ(queue.dequeue(), std::optional<int>(model.front()));
            model.pop_front();
        }
    }
}

TEST(2, "producers and a consumer keep FIFO order in every explored interleaving") {
    explore_producers_and_consumer<SchedTwoLock>();
}

TEST(2, "a dequeue racing the first enqueue sees nothing or the value") {
    const auto report = dsched::explore_dfs([] {
        auto queue = std::make_shared<SchedTwoLock<std::uint64_t>>(4);
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
    });
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "an enqueuer and a dequeuer can both hold their locks at once") {
    LockStats stats;
    active_stats = &stats;
    dsched::explore_dfs(
        [] {
            auto queue =
                std::make_shared<TwoLockQueue<std::uint64_t, TrackingMutex, dsched::Atomic>>(4);
            queue->enqueue(1);
            dsched::Scenario scenario;
            scenario.threads = {[queue] { queue->enqueue(2); }, [queue] { queue->dequeue(); }};
            return scenario;
        },
        {.max_preemptions = 2, .max_runs = 400});
    active_stats = nullptr;
    CHECK_EQ(stats.max_held, 2);
}

TEST(2, "real producers and consumers lose nothing and keep per-producer order") {
    stress_real_threads<TwoLockQueue<std::string>>(2, 2, 1500);
}

TEST(3, "the single-enqueuer queue is FIFO") {
    SingleEnqueuerQueue<int> queue(4);
    CHECK(queue.enqueue(1));
    CHECK(queue.enqueue(2));
    CHECK_EQ(queue.dequeue(), std::optional<int>(1));
    CHECK(queue.enqueue(3));
    CHECK_EQ(queue.dequeue(), std::optional<int>(2));
    CHECK_EQ(queue.dequeue(), std::optional<int>(3));
    CHECK(!queue.dequeue().has_value());
}

TEST(3, "enqueue takes no lock at all") {
    LockStats stats;
    active_stats = &stats;
    SingleEnqueuerQueue<std::uint64_t, TrackingMutex, dsched::Atomic> queue(8);
    for (std::uint64_t i = 1; i <= 8; ++i) {
        CHECK(queue.enqueue(i));
    }
    CHECK_EQ(stats.acquisitions, 0);
    queue.dequeue();
    CHECK_EQ(stats.acquisitions, 1);
    active_stats = nullptr;
}

TEST(3, "one producer and two consumers keep FIFO order in every explored interleaving") {
    const auto report = dsched::explore_dfs(
        [] {
            auto queue = std::make_shared<SchedSingle<std::uint64_t>>(4);
            auto a = std::make_shared<std::vector<std::uint64_t>>();
            auto b = std::make_shared<std::vector<std::uint64_t>>();
            dsched::Scenario scenario;
            scenario.threads = {
                [queue] {
                    CHECK(queue->enqueue(101));
                    CHECK(queue->enqueue(102));
                    CHECK(queue->enqueue(103));
                },
                [queue, a] {
                    if (const auto value = queue->dequeue()) {
                        a->push_back(*value);
                    }
                },
                [queue, b] {
                    if (const auto value = queue->dequeue()) {
                        b->push_back(*value);
                    }
                }};
            scenario.check = [queue, a, b] {
                auto rest = drain(*queue);
                std::vector<std::uint64_t> all = *a;
                all.insert(all.end(), b->begin(), b->end());
                all.insert(all.end(), rest.begin(), rest.end());
                return producers_in_order(rest) && exactly_once(all, {101, 102, 103});
            };
            return scenario;
        },
        {.max_preemptions = 2, .max_runs = 3000});
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.clean());
}

TEST(3, "a real producer and several consumers lose nothing") {
    stress_real_threads<SingleEnqueuerQueue<std::string>>(1, 3, 3000);
}
