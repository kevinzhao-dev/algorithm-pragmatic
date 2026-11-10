#include <algorithm>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"
#include "workload.hpp"

using practice::ols::Growth;

namespace {

template <typename R, typename... Args>
const void* fn(R (*function)(Args...)) {
    return reinterpret_cast<const void*>(function);
}

// Number of activations of fib(n), computed independently of any instrumentation.
long long fib_calls(int n) {
    long long a = 1, b = 1;  // C(0), C(1)
    for (int i = 2; i <= n; ++i) {
        const long long next = 1 + a + b;
        a = b;
        b = next;
    }
    return b;
}

void record_leaf_calls(int n) {
    profiler::start();
    for (int i = 0; i < n; ++i) {
        workload::leaf();
    }
    profiler::stop();
}

}  // namespace

TEST(1, "fib counts every recursive activation") {
    profiler::start();
    workload::fib(10);
    profiler::stop();
    CHECK_EQ(profiler::calls(fn(workload::fib)), fib_calls(10));
    CHECK_EQ(fib_calls(10), 177);
}

TEST(1, "a function never called counts zero") {
    record_leaf_calls(3);
    CHECK_EQ(profiler::calls(fn(workload::leaf)), 3);
    CHECK_EQ(profiler::calls(fn(workload::fib)), 0);
}

TEST(1, "start clears earlier recordings") {
    record_leaf_calls(5);
    CHECK_EQ(profiler::calls(fn(workload::leaf)), 5);
    record_leaf_calls(2);
    CHECK_EQ(profiler::calls(fn(workload::leaf)), 2);
}

TEST(1, "nothing is counted before start or after stop") {
    profiler::start();
    workload::leaf();
    profiler::stop();
    workload::leaf();
    workload::fib(5);
    CHECK_EQ(profiler::calls(fn(workload::leaf)), 1);
    CHECK_EQ(profiler::calls(fn(workload::fib)), 0);
}

TEST(1, "mutual recursion is attributed to each function") {
    profiler::start();
    workload::is_even(7);
    profiler::stop();
    // is_even(7), is_odd(6), is_even(5), is_odd(4), is_even(3), is_odd(2), is_even(1), is_odd(0)
    CHECK_EQ(profiler::calls(fn(workload::is_even)), 4);
    CHECK_EQ(profiler::calls(fn(workload::is_odd)), 4);
}

TEST(1, "helpers that are not instrumented are not counted") {
    profiler::start();
    workload::burn(7);
    profiler::stop();
    CHECK_EQ(profiler::calls(fn(workload::burn)), 0);
}

TEST(1, "counts agree across repeated recordings") {
    for (int n : {0, 1, 2, 7, 15}) {
        INFO("n=", n);
        profiler::start();
        workload::fib(n);
        profiler::stop();
        REQUIRE_EQ(profiler::calls(fn(workload::fib)), fib_calls(n));
    }
}

TEST(2, "the report lists exactly the executed functions, sorted") {
    profiler::start();
    workload::leaf();
    workload::leaf();
    workload::is_even(3);
    profiler::stop();
    const auto lines = profiler::report();
    REQUIRE_EQ(lines.size(), 3);
    CHECK_EQ(lines[0].name, "workload::is_even(int)");
    CHECK_EQ(lines[0].calls, 2);
    CHECK_EQ(lines[1].name, "workload::is_odd(int)");
    CHECK_EQ(lines[1].calls, 2);
    CHECK_EQ(lines[2].name, "workload::leaf()");
    CHECK_EQ(lines[2].calls, 2);
}

TEST(2, "an empty recording has an empty report") {
    profiler::start();
    profiler::stop();
    CHECK(profiler::report().empty());
}

TEST(2, "names are demangled for functions with arguments") {
    profiler::start();
    workload::fib(4);
    profiler::stop();
    const auto lines = profiler::report();
    REQUIRE_EQ(lines.size(), 1);
    CHECK_EQ(lines[0].name, "workload::fib(int)");
    CHECK_EQ(lines[0].calls, fib_calls(4));
}

namespace {

struct VirtualClock {
    VirtualClock() {
        workload::set_now(0);
        profiler::set_clock(&workload::now);
    }
    ~VirtualClock() { profiler::set_clock(nullptr); }
};

}  // namespace

TEST(3, "time is split between a function and its callee") {
    const VirtualClock clock;
    profiler::start();
    workload::outer();
    profiler::stop();
    const auto a = profiler::stats(fn(workload::outer));
    const auto b = profiler::stats(fn(workload::inner));
    CHECK_EQ(a.calls, 1);
    CHECK_EQ(a.exclusive_ticks, 15);
    CHECK_EQ(a.inclusive_ticks, 22);
    CHECK_EQ(b.calls, 1);
    CHECK_EQ(b.exclusive_ticks, 7);
    CHECK_EQ(b.inclusive_ticks, 7);
}

TEST(3, "recursion is not double counted in inclusive time") {
    const VirtualClock clock;
    profiler::start();
    workload::recurse(4);
    profiler::stop();
    const auto stats = profiler::stats(fn(workload::recurse));
    CHECK_EQ(stats.calls, 5);
    CHECK_EQ(stats.exclusive_ticks, 15);  // 5 activations of burn(2) + burn(1)
    CHECK_EQ(stats.inclusive_ticks, 15);  // the outermost activation covers everything once
}

TEST(3, "mutual recursion counts inclusive time per function") {
    const VirtualClock clock;
    profiler::start();
    workload::ping(2);  // ping(2) burns 1, pong(1) burns 2, ping(0) burns 1
    profiler::stop();
    const auto ping = profiler::stats(fn(workload::ping));
    const auto pong = profiler::stats(fn(workload::pong));
    CHECK_EQ(ping.calls, 2);
    CHECK_EQ(pong.calls, 1);
    CHECK_EQ(ping.exclusive_ticks, 2);
    CHECK_EQ(pong.exclusive_ticks, 2);
    CHECK_EQ(ping.inclusive_ticks, 4);
    CHECK_EQ(pong.inclusive_ticks, 3);
}

TEST(3, "two separate top-level calls add up") {
    const VirtualClock clock;
    profiler::start();
    workload::outer();
    workload::outer();
    profiler::stop();
    const auto a = profiler::stats(fn(workload::outer));
    CHECK_EQ(a.calls, 2);
    CHECK_EQ(a.exclusive_ticks, 30);
    CHECK_EQ(a.inclusive_ticks, 44);
}

TEST(3, "the default clock measures real time") {
    profiler::set_clock(nullptr);
    profiler::start();
    workload::fib(18);
    profiler::stop();
    const auto stats = profiler::stats(fn(workload::fib));
    CHECK_GT(stats.inclusive_ticks, 0);
    CHECK_EQ(stats.calls, fib_calls(18));
}

TEST(4, "a loop of leaf calls is linear") {
    const std::vector<int> sizes{64, 128, 256, 512, 1024, 2048, 4096};
    CHECK(profiler::estimate_growth(fn(workload::leaf), &workload::loop_leaves, sizes) ==
          Growth::Linear);
}

TEST(4, "nested loops are quadratic") {
    const std::vector<int> sizes{16, 32, 64, 128, 256, 512};
    CHECK(profiler::estimate_growth(fn(workload::leaf), &workload::nested_leaves, sizes) ==
          Growth::Quadratic);
}

TEST(4, "merge sort comparisons grow like n log n") {
    const std::vector<int> sizes{64, 128, 256, 512, 1024, 2048, 4096};
    CHECK(profiler::estimate_growth(fn(workload::compare), &workload::sort_counted, sizes) ==
          Growth::NLogN);
}

TEST(4, "naive fib is exponential") {
    const std::vector<int> sizes{10, 12, 14, 16, 18, 20, 22};
    CHECK(profiler::estimate_growth(fn(workload::fib), &workload::run_fib, sizes) ==
          Growth::Exponential);
}
