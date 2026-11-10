#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "practice.hpp"

struct TraceOp {
    enum class Kind { Alloc, Free };
    Kind kind;
    int id;
    std::size_t size;  // meaningful for Alloc only
};

using Trace = std::vector<TraceOp>;

struct TraceReport {
    std::size_t peak_live_bytes = 0;  // M
    std::size_t heap_bytes = 0;       // H
    std::size_t failed_allocs = 0;
    double utilization = 0.0;  // U
};

// Stage 1
inline TraceReport run_fixed_block(const Trace& trace, std::size_t block_size) { PRACTICE_TODO(); }
inline double performance_index(double utilization, double throughput, double libc_throughput,
                                double weight) {
    PRACTICE_TODO();
}

// Stage 2
inline double objective(const TraceReport& report) { PRACTICE_TODO(); }
inline std::size_t autotune(const Trace& trace, std::span<const std::size_t> candidates) {
    PRACTICE_TODO();
}

// Stage 3
inline std::size_t autotune_robust(std::span<const Trace> traces,
                                   std::span<const std::size_t> candidates) {
    PRACTICE_TODO();
}
