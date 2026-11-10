// 01_bench.hpp — same-binary timing helpers for the 6.172 performance exercises.
//
//     double ratio = bench::speedup(5, [&] { slow(); }, [&] { fast(); });
//     CHECK_GE(ratio, 3.0);
//
// Each side runs `trials` times in alternation and only the fastest run of each side counts.
// The fastest run is the one least disturbed by other processes, and alternating keeps a slow
// patch of machine load from landing entirely on one side.
#pragma once

#include <algorithm>
#include <chrono>

namespace bench {

// Forces the compiler to treat `value` as observed.
template <typename T>
inline void do_not_optimize(const T& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

// Tells the compiler that any memory may have been read or written here.
inline void clobber_memory() {
    asm volatile("" : : : "memory");
}

template <typename F>
double once_ms(F&& body) {
    const auto start = std::chrono::steady_clock::now();
    body();
    const auto stop = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

template <typename F>
double best_ms(int trials, F&& body) {
    double best = once_ms(body);
    for (int i = 1; i < trials; ++i) {
        best = std::min(best, once_ms(body));
    }
    return best;
}

// How many times faster `candidate` is than `baseline`.
template <typename Baseline, typename Candidate>
double speedup(int trials, Baseline&& baseline, Candidate&& candidate) {
    double best_baseline = once_ms(baseline);
    double best_candidate = once_ms(candidate);
    for (int i = 1; i < trials; ++i) {
        best_candidate = std::min(best_candidate, once_ms(candidate));
        best_baseline = std::min(best_baseline, once_ms(baseline));
    }
    return best_baseline / std::max(best_candidate, 1e-6);
}

}  // namespace bench
