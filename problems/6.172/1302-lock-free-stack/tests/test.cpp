#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "08_dsched.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
namespace dsched = practice::dsched;

namespace {

using Value = std::optional<std::uint64_t>;
using SchedStack = LockFreeStack<dsched::Atomic>;

// Pops what is left, in pop order. The bound keeps a corrupted, cyclic list from hanging the test.
std::vector<std::uint64_t> drain(SchedStack& stack) {
    std::vector<std::uint64_t> values;
    while (values.size() < 32) {
        const auto value = stack.pop();
        if (!value) {
            break;
        }
        values.push_back(*value);
    }
    return values;
}

// Every value must be accounted for exactly once: popped by a thread or still in the stack.
bool exactly_once(std::vector<std::uint64_t> seen, std::vector<std::uint64_t> expected) {
    std::sort(seen.begin(), seen.end());
    std::sort(expected.begin(), expected.end());
    return seen == expected;
}

}  // namespace

TEST(1, "values come back in last-in first-out order") {
    LockFreeStack<> stack(8);
    CHECK(stack.push(1));
    CHECK(stack.push(2));
    CHECK(stack.push(3));
    CHECK_EQ(stack.pop(), Value(3));
    CHECK_EQ(stack.pop(), Value(2));
    CHECK(stack.push(4));
    CHECK_EQ(stack.pop(), Value(4));
    CHECK_EQ(stack.pop(), Value(1));
}

TEST(1, "popping an empty stack returns nothing") {
    LockFreeStack<> stack(4);
    CHECK(!stack.pop().has_value());
    stack.push(9);
    CHECK_EQ(stack.pop(), Value(9));
    CHECK(!stack.pop().has_value());
}

TEST(1, "push fails when every node is in use and recovers after a pop") {
    LockFreeStack<> stack(2);
    CHECK(stack.push(1));
    CHECK(stack.push(2));
    CHECK(!stack.push(3));
    CHECK_EQ(stack.pop(), Value(2));
    CHECK(stack.push(3));
    CHECK_EQ(stack.pop(), Value(3));
}

TEST(1, "a random workload matches a vector") {
    Rng rng;
    LockFreeStack<> stack(64);
    std::vector<std::uint64_t> model;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        if (model.empty() || (model.size() < 64 && rng.coin())) {
            const auto value = static_cast<std::uint64_t>(rng.uniform(0, 1'000'000));
            REQUIRE(stack.push(value));
            model.push_back(value);
        } else {
            REQUIRE_EQ(stack.pop(), Value(model.back()));
            model.pop_back();
        }
    }
}

TEST(2, "two concurrent pushes both stay in the stack") {
    const auto report = dsched::explore_dfs([] {
        auto stack = std::make_shared<SchedStack>(4);
        dsched::Scenario scenario;
        scenario.threads = {[stack] { CHECK(stack->push(1)); }, [stack] { CHECK(stack->push(2)); }};
        scenario.check = [stack] { return exactly_once(drain(*stack), {1, 2}); };
        return scenario;
    });
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "a concurrent push and pop lose nothing") {
    const auto report = dsched::explore_dfs([] {
        auto stack = std::make_shared<SchedStack>(4);
        stack->push(1);
        auto popped = std::make_shared<Value>();
        dsched::Scenario scenario;
        scenario.threads = {[stack] { stack->push(2); }, [stack, popped] { *popped = stack->pop(); }};
        scenario.check = [stack, popped] {
            auto rest = drain(*stack);
            if (popped->has_value()) {
                rest.push_back(**popped);
            }
            return exactly_once(rest, {1, 2});
        };
        return scenario;
    });
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(2, "two poppers never get the same value") {
    const auto report = dsched::explore_dfs([] {
        auto stack = std::make_shared<SchedStack>(4);
        stack->push(1);
        stack->push(2);
        auto a = std::make_shared<Value>();
        auto b = std::make_shared<Value>();
        dsched::Scenario scenario;
        scenario.threads = {[stack, a] { *a = stack->pop(); }, [stack, b] { *b = stack->pop(); }};
        scenario.check = [a, b] { return a->has_value() && b->has_value() && **a != **b; };
        return scenario;
    });
    INFO("bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(3, "a recycled head does not fool a stale pop") {
    const auto report = dsched::explore_dfs(
        [] {
            // Only two nodes, reused first-in first-out, so the push below recycles the old head.
            auto stack = std::make_shared<SchedStack>(2);
            stack->push(1);
            stack->push(2);
            auto a = std::make_shared<Value>();
            auto b = std::make_shared<std::vector<std::uint64_t>>();
            dsched::Scenario scenario;
            scenario.threads = {
                [stack, a] { *a = stack->pop(); },
                [stack, b] {
                    for (int i = 0; i < 2; ++i) {
                        if (const auto value = stack->pop()) {
                            b->push_back(*value);
                        }
                    }
                    CHECK(stack->push(3));
                }};
            scenario.check = [stack, a, b] {
                auto all = drain(*stack);
                all.insert(all.end(), b->begin(), b->end());
                if (a->has_value()) {
                    all.push_back(**a);
                }
                return exactly_once(all, {1, 2, 3});
            };
            return scenario;
        },
        {.max_preemptions = 2});
    INFO("runs ", report.runs, " bad schedule ", report.bad_trace);
    CHECK(report.exhausted);
    CHECK(report.clean());
}

TEST(3, "several threads push and pop without losing values") {
    constexpr int kThreads = 4;
    constexpr int kOperations = 3000;
    LockFreeStack<> stack(kThreads * kOperations + 8);
    std::mutex results_mutex;
    std::vector<std::uint64_t> popped;
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            std::vector<std::uint64_t> mine;
            for (int i = 0; i < kOperations; ++i) {
                const std::uint64_t value = static_cast<std::uint64_t>(t) * 1'000'000 +
                                            static_cast<std::uint64_t>(i) + 1;
                CHECK(stack.push(value));
                if (i % 3 == 0) {
                    if (const auto got = stack.pop()) {
                        mine.push_back(*got);
                    }
                }
            }
            const std::lock_guard lock(results_mutex);
            popped.insert(popped.end(), mine.begin(), mine.end());
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    while (popped.size() < kThreads * kOperations + 16) {
        const auto value = stack.pop();
        if (!value) {
            break;
        }
        popped.push_back(*value);
    }
    CHECK_EQ(popped.size(), kThreads * kOperations);
    std::sort(popped.begin(), popped.end());
    CHECK(std::adjacent_find(popped.begin(), popped.end()) == popped.end());
}
