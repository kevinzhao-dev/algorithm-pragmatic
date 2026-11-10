#include <atomic>
#include <numeric>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Exact accounting of the fib recursion rules, computed by plain recursion.
struct FibModel {
    long long work = 0;
    long long span = 0;
    long long spawns = 0;
};

FibModel fib_model(int n, int cutoff) {
    if (n <= 1) {
        return {1, 1, 0};
    }
    if (n <= cutoff) {
        const FibModel a = fib_model(n - 1, cutoff);
        const FibModel b = fib_model(n - 2, cutoff);
        return {1 + a.work + b.work, 1 + a.span + b.span, 0};
    }
    const FibModel a = fib_model(n - 1, cutoff);
    const FibModel b = fib_model(n - 2, cutoff);
    return {2 + a.work + b.work, 2 + std::max(a.span, b.span), 1 + a.spawns + b.spawns};
}

long long fib_value(int n) {
    long long a = 0;
    long long b = 1;
    for (int i = 0; i < n; ++i) {
        const long long c = a + b;
        a = b;
        b = c;
    }
    return a;
}

struct WorkerCount {
    unsigned count;
    explicit WorkerCount(unsigned c) : count(c) { practice::set_workers(c); }
};

}  // namespace

TEST(1, "series adds both work and span") {
    const Cost c = series(Cost{3, 1}, Cost{4, 2});
    CHECK_EQ(c.work, 7);
    CHECK_EQ(c.span, 3);
}

TEST(1, "parallel adds work and takes the longer span") {
    const Cost c = parallel(Cost{3, 1}, Cost{4, 2});
    CHECK_EQ(c.work, 7);
    CHECK_EQ(c.span, 2);
    const Cost d = parallel(Cost{10, 10}, Cost{1, 1});
    CHECK_EQ(d.span, 10);
}

TEST(1, "parallelism is work over span") {
    CHECK_NEAR((Cost{8, 2}).parallelism(), 4.0, 1e-9);
    CHECK_NEAR((Cost{0, 0}).parallelism(), 1.0, 1e-9);
}

TEST(2, "charge accumulates on the current strand") {
    Meter meter;
    meter.charge();
    meter.charge(4);
    CHECK_EQ(meter.cost().work, 5);
    CHECK_EQ(meter.cost().span, 5);
    CHECK_EQ(meter.spawns(), 0);
}

TEST(2, "fork2 runs both branches and charges one spawn") {
    Meter meter;
    int calls = 0;
    meter.fork2([&] { calls += 1; meter.charge(3); }, [&] { calls += 10; meter.charge(5); });
    CHECK_EQ(calls, 11);
    CHECK_EQ(meter.spawns(), 1);
    CHECK_EQ(meter.cost().work, 3 + 5 + 1);
    CHECK_EQ(meter.cost().span, 5 + 1);
}

TEST(2, "work done after the join goes back to the parent strand") {
    Meter meter;
    meter.charge(2);
    meter.fork2([&] { meter.charge(7); }, [&] { meter.charge(1); });
    meter.charge(3);
    CHECK_EQ(meter.cost().work, 2 + 1 + 7 + 1 + 3);
    CHECK_EQ(meter.cost().span, 2 + 1 + 7 + 3);
}

TEST(2, "nesting composes") {
    Meter meter;
    meter.fork2(
        [&] {
            meter.fork2([&] { meter.charge(1); }, [&] { meter.charge(2); });
        },
        [&] { meter.charge(10); });
    CHECK_EQ(meter.spawns(), 2);
    CHECK_EQ(meter.cost().work, 1 + 1 + 1 + 2 + 10);
    CHECK_EQ(meter.cost().span, 1 + 10);
}

TEST(2, "parallel_for splits in halves down to the grain") {
    Meter meter;
    std::vector<int> seen(8, 0);
    meter.parallel_for(0, 8, [&](std::size_t i) {
        ++seen[i];
        meter.charge();
    });
    CHECK_EQ(seen, std::vector<int>(8, 1));
    CHECK_EQ(meter.spawns(), 7);
    CHECK_EQ(meter.cost().work, 8 + 7);
    CHECK_EQ(meter.cost().span, 3 + 1);
}

TEST(2, "parallel_for honours the grain and handles empty ranges") {
    Meter meter;
    int count = 0;
    meter.parallel_for(5, 5, [&](std::size_t) { ++count; });
    meter.parallel_for(0, 4, [&](std::size_t) { ++count; }, 4);
    CHECK_EQ(count, 4);
    CHECK_EQ(meter.spawns(), 0);
    Meter coarse;
    coarse.parallel_for(0, 1024, [&](std::size_t) { coarse.charge(); }, 16);
    CHECK_EQ(coarse.spawns(), 63);
}

TEST(3, "fib returns correct values under the meter") {
    for (int n = 0; n <= 20; ++n) {
        Meter meter;
        CHECK_EQ(fib(meter, n, 5), fib_value(n));
    }
}

TEST(3, "the accounting matches the rules exactly") {
    for (const int cutoff : {1, 3, 5, 10, 15, 30}) {
        for (const int n : {0, 1, 2, 5, 10, 18, 22}) {
            Meter meter;
            fib(meter, n, cutoff);
            const FibModel model = fib_model(n, cutoff);
            INFO("n ", n, " cutoff ", cutoff);
            CHECK_EQ(meter.cost().work, model.work);
            CHECK_EQ(meter.cost().span, model.span);
            CHECK_EQ(meter.spawns(), model.spawns);
        }
    }
}

TEST(3, "coarsening removes spawns at a modest cost in span") {
    Meter fine;
    fib(fine, 24, 1);
    Meter coarse;
    fib(coarse, 24, 12);
    // Both versions make exactly the same function calls.
    CHECK_EQ(fine.cost().work - fine.spawns(), coarse.cost().work - coarse.spawns());
    CHECK_GT(fine.spawns(), coarse.spawns() * 50);
    // The serial base cases lengthen the span, but plenty of parallelism remains.
    CHECK_GT(coarse.cost().span, fine.cost().span);
    CHECK_GT(coarse.cost().parallelism(), 32.0);
}

TEST(4, "fib gives the same answer on the real pool") {
    practice::set_workers(4);
    ForkJoinExec exec;
    for (const int n : {0, 1, 2, 10, 25}) {
        CHECK_EQ(fib(exec, n, 12), fib_value(n));
    }
}

TEST(4, "parallel_sum is race-free and correct for any worker count") {
    Rng rng;
    const std::vector<int> data = rng.ints(100'000, -1000, 1000);
    const long long expected = std::accumulate(data.begin(), data.end(), 0LL);
    ForkJoinExec exec;
    for (const unsigned workers : {1U, 4U, 8U}) {
        practice::set_workers(workers);
        for (const std::size_t grain : {1, 7, 1000}) {
            INFO("workers ", workers, " grain ", grain);
            CHECK_EQ(parallel_sum(exec, data, grain), expected);
        }
    }
    CHECK_EQ(parallel_sum(exec, std::span<const int>(), 8), 0);
}

TEST(4, "parallel_sum cost is linear work and logarithmic span") {
    const std::vector<int> data(1 << 12, 1);
    Meter meter;
    CHECK_EQ(parallel_sum(meter, data, 1), data.size());
    CHECK_EQ(meter.cost().work, static_cast<long long>(2 * data.size() - 1));
    CHECK_LE(meter.cost().span, 14);
}

TEST(4, "transpose_in_place is correct on the real pool") {
    Rng rng;
    const std::size_t n = 300;
    std::vector<int> grid(n * n);
    std::iota(grid.begin(), grid.end(), 0);
    std::vector<int> expected(n * n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            expected[j * n + i] = static_cast<int>(i * n + j);
        }
    }
    practice::set_workers(6);
    ForkJoinExec exec;
    transpose_in_place(exec, grid, n, 8);
    CHECK_EQ(grid, expected);
}

TEST(4, "transpose handles the smallest shapes") {
    ForkJoinExec exec;
    std::vector<int> one = {42};
    transpose_in_place(exec, one, 1, 1);
    CHECK_EQ(one, std::vector<int>{42});
    std::vector<int> none;
    transpose_in_place(exec, none, 0, 1);
    CHECK(none.empty());
}

TEST(4, "transpose work is quadratic and span logarithmic under the meter") {
    const std::size_t n = 128;
    std::vector<int> grid(n * n, 0);
    Meter meter;
    transpose_in_place(meter, grid, n, 1);
    // n(n-1)/2 swaps plus one fork per internal node of every parallel_for tree.
    CHECK_GE(meter.cost().work, static_cast<long long>(n * (n - 1) / 2));
    CHECK_LE(meter.cost().span, 30);
}

TEST(5, "speedup bounds follow the work and span laws") {
    const Cost cost{1000, 10};  // parallelism 100
    const SpeedupBounds few = speedup_bounds(cost, 4);
    CHECK_NEAR(few.upper, 4.0, 1e-9);
    CHECK_NEAR(few.lower, 1000.0 / (250.0 + 10.0), 1e-9);
    const SpeedupBounds many = speedup_bounds(cost, 1000);
    CHECK_NEAR(many.upper, 100.0, 1e-9);
    CHECK_LE(many.lower, many.upper);
    CHECK_GE(many.lower, 0.0);
}

TEST(5, "bounds are ordered and within the physical limits") {
    Rng rng;
    for (int round = 0; round < 2000; ++round) {
        const long long span = rng.uniform(1, 1000);
        const long long work = rng.uniform(span, span * 1000);
        const int p = static_cast<int>(rng.uniform(1, 1 << 20));
        const SpeedupBounds b = speedup_bounds(Cost{work, span}, p);
        INFO("work ", work, " span ", span, " p ", p);
        REQUIRE(b.lower <= b.upper + 1e-9);
        REQUIRE(b.lower <= static_cast<double>(p) + 1e-9);
        REQUIRE(b.upper <= static_cast<double>(work) / static_cast<double>(span) + 1e-9);
        REQUIRE(b.lower > 0);
    }
}
