#include <algorithm>
#include <limits>
#include <vector>

#include "forkjoin.hpp"
#include "practice.hpp"
#include "solution.hpp"

namespace {

std::uint64_t spawn_count = 0;

}  // namespace

extern "C" void __cyg_profile_func_enter(void* this_fn, void* /*call_site*/) {
    if (this_fn == reinterpret_cast<void*>(&workload::spawn_marker)) {
        ++spawn_count;
    }
}

extern "C" void __cyg_profile_func_exit(void* /*this_fn*/, void* /*call_site*/) {}

RunCounts count_run(int n, int base) {
    practice::set_workers(1);
    const std::uint64_t units_before = workload::work_units();
    spawn_count = 0;
    workload::fib_spawn(n, base);
    return RunCounts{workload::work_units() - units_before, spawn_count};
}

Sample sample_virtual(int n, int base) {
    const std::uint64_t ticks_before = workload::ticks();
    const RunCounts counts = count_run(n, base);
    return Sample{static_cast<double>(counts.instructions), static_cast<double>(counts.spawns),
                  static_cast<double>(workload::ticks() - ticks_before)};
}

CostModel fit_spawn_cost(std::span<const Sample> samples) {
    std::vector<std::vector<double>> rows;
    std::vector<double> times;
    for (const Sample& sample : samples) {
        rows.push_back({sample.instructions, sample.spawns});
        times.push_back(sample.time);
    }
    const auto beta = practice::ols::solve(rows, times);
    return CostModel{beta[0], beta[1]};
}

double time_fib_spawn_ns(int n, int base, int repeats) {
    practice::set_workers(1);
    double best = std::numeric_limits<double>::infinity();
    for (int i = 0; i < repeats; ++i) {
        const practice::Stopwatch watch;
        static volatile long long sink;
        sink = workload_plain::fib_spawn(n, base);
        best = std::min(best, watch.elapsed_ms() * 1e6);
    }
    return best;
}

std::vector<Input> design_inputs() {
    // Raising the cut-off at a fixed n removes spawns while the serial work stays the same.
    return {{22, 22}, {22, 18}, {22, 14}, {22, 10}, {22, 6}, {21, 19}, {21, 12}, {20, 8}, {20, 15}, {23, 20}};
}

double measure_spawn_cost(std::span<const Input> inputs, int repeats) {
    std::vector<std::vector<double>> rows;
    std::vector<double> times;
    for (const auto& [n, base] : inputs) {
        const RunCounts counts = count_run(n, base);
        rows.push_back({static_cast<double>(counts.instructions), static_cast<double>(counts.spawns)});
        times.push_back(time_fib_spawn_ns(n, base, repeats));
    }
    return practice::ols::solve(rows, times)[1];
}

double call_cost_ns() {
    constexpr int kCalls = 2'000'000;
    double best = std::numeric_limits<double>::infinity();
    for (int round = 0; round < 7; ++round) {
        const practice::Stopwatch watch;
        for (int i = 0; i < kCalls; ++i) {
            workload_plain::empty_call();
        }
        best = std::min(best, watch.elapsed_ms() * 1e6 / kCalls);
    }
    return best;
}
