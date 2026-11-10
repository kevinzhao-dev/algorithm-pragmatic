#include <algorithm>
#include <map>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t kFloor = 40 * 1024;

TraceOp alloc(int id, std::size_t size) { return {TraceOp::Kind::Alloc, id, size}; }
TraceOp release(int id) { return {TraceOp::Kind::Free, id, 0}; }

// Many same-sized live objects, large enough to clear the 40 KiB floor.
Trace same_size_trace(int count, std::size_t size) {
    Trace trace;
    for (int i = 0; i < count; ++i) {
        trace.push_back(alloc(i, size));
    }
    return trace;
}

std::vector<std::size_t> powers_of_two() {
    std::vector<std::size_t> sizes;
    for (std::size_t size = 32; size <= 32768; size *= 2) {
        sizes.push_back(size);
    }
    return sizes;
}

// Independent reference implementation of the accounting rules.
TraceReport model(const Trace& trace, std::size_t block_size) {
    TraceReport report;
    std::map<int, std::size_t> live;
    std::size_t live_bytes = 0;
    std::size_t idle = 0;
    std::size_t blocks = 0;
    for (const auto& op : trace) {
        if (op.kind == TraceOp::Kind::Alloc) {
            if (op.size > block_size) {
                ++report.failed_allocs;
            } else {
                live[op.id] = op.size;
                live_bytes += op.size;
                if (idle > 0) {
                    --idle;
                } else {
                    ++blocks;
                }
            }
        } else if (live.contains(op.id)) {
            live_bytes -= live[op.id];
            live.erase(op.id);
            ++idle;
        }
        report.peak_live_bytes = std::max(report.peak_live_bytes, live_bytes);
    }
    report.heap_bytes = blocks * block_size;
    report.utilization = static_cast<double>(std::max(report.peak_live_bytes, kFloor)) /
                         static_cast<double>(std::max(report.heap_bytes, kFloor));
    return report;
}

}  // namespace

TEST(1, "an empty trace uses nothing") {
    const TraceReport report = run_fixed_block({}, 1024);
    CHECK_EQ(report.peak_live_bytes, 0);
    CHECK_EQ(report.heap_bytes, 0);
    CHECK_EQ(report.failed_allocs, 0);
}

TEST(1, "same-size objects at the block size fill the heap exactly") {
    const TraceReport report = run_fixed_block(same_size_trace(100, 1024), 1024);
    CHECK_EQ(report.peak_live_bytes, 102'400);
    CHECK_EQ(report.heap_bytes, 102'400);
    CHECK_EQ(report.failed_allocs, 0);
    CHECK_NEAR(report.utilization, 1.0, 1e-12);
}

TEST(1, "half-sized objects give one-half utilization") {
    const TraceReport report = run_fixed_block(same_size_trace(200, 512), 1024);
    CHECK_EQ(report.heap_bytes, 200 * 1024);
    CHECK_NEAR(report.utilization, 0.5, 1e-12);
}

TEST(1, "tiny traces are clamped to the 40 KiB floor") {
    const TraceReport report = run_fixed_block(same_size_trace(2, 100), 1024);
    CHECK_NEAR(report.utilization, 1.0, 1e-12);
}

TEST(1, "freed blocks are reused before the heap grows") {
    Trace trace = same_size_trace(100, 1024);
    trace.push_back(release(0));
    trace.push_back(release(1));
    trace.push_back(alloc(100, 1000));
    trace.push_back(alloc(101, 900));
    trace.push_back(alloc(102, 800));
    const TraceReport report = run_fixed_block(trace, 1024);
    CHECK_EQ(report.heap_bytes, 101 * 1024);
}

TEST(1, "utilization keeps the peak over the whole trace") {
    Trace trace = same_size_trace(100, 1024);
    for (int i = 0; i < 99; ++i) {
        trace.push_back(release(i));
    }
    const TraceReport report = run_fixed_block(trace, 1024);
    CHECK_NEAR(report.utilization, 1.0, 1e-12);
    CHECK_EQ(report.peak_live_bytes, 102'400);
}

TEST(1, "oversized requests fail and their frees are ignored") {
    Trace trace;
    trace.push_back(alloc(0, 2048));
    trace.push_back(release(0));
    trace.push_back(alloc(1, 1024));
    const TraceReport report = run_fixed_block(trace, 1024);
    CHECK_EQ(report.failed_allocs, 1);
    CHECK_EQ(report.heap_bytes, 1024);
    CHECK_EQ(report.peak_live_bytes, 1024);
}

TEST(1, "performance index matches the formula") {
    CHECK_NEAR(performance_index(0.8, 50, 100, 0.5), 0.5 * 0.8 + 0.5 * 0.5, 1e-12);
    CHECK_NEAR(performance_index(1.0, 300, 100, 0.25), 0.25 + 0.75, 1e-12);
    CHECK_NEAR(performance_index(0.2, 10, 100, 1.0), 0.2, 1e-12);
}

TEST(1, "random traces match an independent model") {
    Rng rng;
    for (int trial = 0; trial < 30; ++trial) {
        INFO("trial ", trial);
        const auto block = static_cast<std::size_t>(rng.uniform(64, 8192));
        Trace trace;
        std::vector<int> live;
        int next_id = 0;
        const int steps = static_cast<int>(rng.uniform(500, 3000));
        for (int step = 0; step < steps; ++step) {
            if (live.empty() || rng.coin(0.6)) {
                trace.push_back(alloc(next_id, static_cast<std::size_t>(rng.uniform(1, 9000))));
                live.push_back(next_id);
                ++next_id;
            } else {
                const auto i = static_cast<std::size_t>(
                    rng.uniform(0, static_cast<std::int64_t>(live.size()) - 1));
                trace.push_back(release(live[i]));
                live[i] = live.back();
                live.pop_back();
            }
        }
        const TraceReport got = run_fixed_block(trace, block);
        const TraceReport want = model(trace, block);
        REQUIRE_EQ(got.peak_live_bytes, want.peak_live_bytes);
        REQUIRE_EQ(got.heap_bytes, want.heap_bytes);
        REQUIRE_EQ(got.failed_allocs, want.failed_allocs);
        CHECK_NEAR(got.utilization, want.utilization, 1e-9);
    }
}

TEST(2, "objective is zero with failures and utilization otherwise") {
    TraceReport report;
    report.failed_allocs = 3;
    report.utilization = 0.9;
    CHECK_NEAR(objective(report), 0.0, 1e-12);
    report.failed_allocs = 0;
    CHECK_NEAR(objective(report), 0.9, 1e-12);
}

TEST(2, "autotune recovers the block size hidden in the trace") {
    const auto candidates = powers_of_two();
    CHECK_EQ(autotune(same_size_trace(1000, 1024), candidates), 1024);
    CHECK_EQ(autotune(same_size_trace(1000, 4096), candidates), 4096);
}

TEST(2, "autotune rounds a mixed trace up to the largest size") {
    Trace trace = same_size_trace(1000, 3000);
    trace.push_back(alloc(5000, 800));
    const auto candidates = powers_of_two();
    CHECK_EQ(autotune(trace, candidates), 4096);
}

TEST(2, "one oversized request drags the block size up with it") {
    Trace trace = same_size_trace(1000, 500);
    trace.push_back(alloc(9999, 20'000));
    const auto candidates = powers_of_two();
    CHECK_EQ(autotune(trace, candidates), 32'768);
}

TEST(2, "candidates are not sorted and the best sits in the middle") {
    const Trace trace = same_size_trace(1000, 300);
    const std::vector<std::size_t> shuffled{4096, 512, 16384, 1024, 8192, 256, 2048};
    CHECK_EQ(autotune(trace, shuffled), 512);
}

TEST(2, "ties go to the smallest block") {
    // Under the 40 KiB floor every workable block size scores 1.
    const Trace trace = same_size_trace(2, 100);
    const auto candidates = powers_of_two();
    CHECK_EQ(autotune(trace, candidates), 128);
}

TEST(3, "tuning on one trace can overfit") {
    const Trace big = same_size_trace(1000, 1024);
    const Trace other = same_size_trace(1000, 2048);
    const auto candidates = powers_of_two();

    const std::size_t overfit = autotune(big, candidates);
    CHECK_EQ(overfit, 1024);
    CHECK_NEAR(objective(run_fixed_block(other, overfit)), 0.0, 1e-12);

    const std::vector<Trace> both{big, other};
    const std::size_t robust = autotune_robust(both, candidates);
    CHECK_EQ(robust, 2048);
    CHECK_GT(objective(run_fixed_block(other, robust)), 0.9);
}

TEST(3, "the robust choice maximizes the worst trace") {
    const Trace small = same_size_trace(2000, 100);
    const Trace large = same_size_trace(1000, 1000);
    const std::vector<Trace> both{small, large};
    const auto candidates = powers_of_two();
    // Anything below 1024 fails the large trace, so its worst case is zero.
    CHECK_EQ(autotune_robust(both, candidates), 1024);
}

TEST(3, "robust tuning breaks ties toward the smallest block") {
    const std::vector<Trace> both{same_size_trace(2, 100), same_size_trace(2, 200)};
    const auto candidates = powers_of_two();
    CHECK_EQ(autotune_robust(both, candidates), 256);
}
