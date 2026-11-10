#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <span>
#include <unordered_map>
#include <vector>

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

inline TraceReport run_fixed_block(const Trace& trace, std::size_t block_size) {
    constexpr std::size_t kFloor = 40 * 1024;

    TraceReport report;
    std::unordered_map<int, std::size_t> live;  // id -> requested bytes
    std::size_t live_bytes = 0;
    std::size_t blocks = 0;
    std::size_t idle = 0;

    for (const TraceOp& op : trace) {
        if (op.kind == TraceOp::Kind::Alloc) {
            if (op.size > block_size) {
                ++report.failed_allocs;
            } else {
                if (idle > 0) {
                    --idle;
                } else {
                    ++blocks;
                }
                live[op.id] = op.size;
                live_bytes += op.size;
            }
        } else if (const auto found = live.find(op.id); found != live.end()) {
            live_bytes -= found->second;
            live.erase(found);
            ++idle;
        }
        report.peak_live_bytes = std::max(report.peak_live_bytes, live_bytes);
    }
    report.heap_bytes = blocks * block_size;
    report.utilization = static_cast<double>(std::max(report.peak_live_bytes, kFloor)) /
                         static_cast<double>(std::max(report.heap_bytes, kFloor));
    return report;
}

inline double performance_index(double utilization, double throughput, double libc_throughput,
                                double weight) {
    return weight * utilization + (1.0 - weight) * std::min(1.0, throughput / libc_throughput);
}

inline double objective(const TraceReport& report) {
    return report.failed_allocs > 0 ? 0.0 : report.utilization;
}

inline std::size_t autotune(const Trace& trace, std::span<const std::size_t> candidates) {
    std::size_t best = candidates.front();
    double best_score = -1.0;
    for (const std::size_t candidate : candidates) {
        const double score = objective(run_fixed_block(trace, candidate));
        if (score > best_score || (score == best_score && candidate < best)) {
            best = candidate;
            best_score = score;
        }
    }
    return best;
}

inline std::size_t autotune_robust(std::span<const Trace> traces,
                                   std::span<const std::size_t> candidates) {
    std::size_t best = candidates.front();
    double best_score = -1.0;
    for (const std::size_t candidate : candidates) {
        double worst = std::numeric_limits<double>::infinity();
        for (const Trace& trace : traces) {
            worst = std::min(worst, objective(run_fixed_block(trace, candidate)));
        }
        if (worst > best_score || (worst == best_score && candidate < best)) {
            best = candidate;
            best_score = worst;
        }
    }
    return best;
}
