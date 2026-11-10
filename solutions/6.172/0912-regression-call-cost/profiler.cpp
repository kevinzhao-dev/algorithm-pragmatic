#include <algorithm>
#include <limits>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

namespace {

std::uint64_t call_count = 0;

template <typename F>
double fastest_run_ns(int repeats, F run) {
    double best = std::numeric_limits<double>::infinity();
    for (int i = 0; i < repeats; ++i) {
        const practice::Stopwatch watch;
        run();
        best = std::min(best, watch.elapsed_ms() * 1e6);
    }
    return best;
}

}  // namespace

extern "C" void __cyg_profile_func_enter(void* this_fn, void* /*call_site*/) {
    if (this_fn == reinterpret_cast<void*>(&workload::fib_base)) {
        ++call_count;
    }
}

extern "C" void __cyg_profile_func_exit(void* /*this_fn*/, void* /*call_site*/) {}

RunCounts count_run(int n, int base) {
    const std::uint64_t units_before = workload::work_units();
    call_count = 0;
    workload::fib_base(n, base);
    return RunCounts{workload::work_units() - units_before, call_count};
}

Sample sample_virtual(int n, int base) {
    const std::uint64_t ticks_before = workload::ticks();
    const RunCounts counts = count_run(n, base);
    return Sample{static_cast<double>(counts.instructions), static_cast<double>(counts.calls),
                  static_cast<double>(workload::ticks() - ticks_before)};
}

CostModel fit_cost_model(std::span<const Sample> samples) {
    std::vector<std::vector<double>> rows;
    std::vector<double> times;
    for (const Sample& sample : samples) {
        rows.push_back({sample.instructions, sample.calls});
        times.push_back(sample.time);
    }
    const auto beta = practice::ols::solve(rows, times);
    return CostModel{beta[0], beta[1]};
}

double counts_correlation(std::span<const Sample> samples) {
    std::vector<double> instructions, calls;
    for (const Sample& sample : samples) {
        instructions.push_back(sample.instructions);
        calls.push_back(sample.calls);
    }
    return practice::ols::correlation(instructions, calls);
}

std::vector<Input> design_inputs() {
    // Varying base at a fixed n trades instructions against calls at nearly constant work.
    return {{26, 2}, {26, 6}, {26, 10}, {26, 13}, {24, 2}, {24, 7}, {24, 12}, {22, 3}, {22, 9}, {20, 5}};
}

double time_fib_ns(int n, int base, int repeats) {
    return fastest_run_ns(repeats, [&] {
        static volatile long long sink;
        sink = workload_plain::fib_base(n, base);
    });
}

CostModel measure_real_cost(std::span<const Input> inputs, int repeats) {
    std::vector<Sample> samples;
    for (const auto& [n, base] : inputs) {
        const RunCounts counts = count_run(n, base);
        samples.push_back({static_cast<double>(counts.instructions), static_cast<double>(counts.calls),
                           time_fib_ns(n, base, repeats)});
    }
    return fit_cost_model(samples);
}
