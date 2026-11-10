#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <random>
#include <span>
#include <utility>
#include <vector>

struct QuicksortTrace {
    std::vector<std::uint32_t> pivot_comparisons;
    std::uint64_t total_comparisons = 0;
};

struct BoundReport {
    int trials = 0;
    int element_violations = 0;
    int total_violations = 0;
    double mean_total = 0.0;
};

inline std::size_t good_pivot_count(std::size_t m, std::size_t rank) {
    std::size_t good = 0;
    for (std::size_t pivot = 0; pivot < m; ++pivot) {
        const std::size_t next = rank < pivot ? pivot : (rank > pivot ? m - pivot - 1 : 0);
        if (pivot == rank || 4 * next <= 3 * m) {
            ++good;
        }
    }
    return good;
}

namespace analysis_detail {

template <typename T, typename Rng>
void traced_sort(std::span<T> values, std::span<std::uint32_t> origin, QuicksortTrace& trace, Rng& rng) {
    while (values.size() > 1) {
        const std::size_t m = values.size();
        const std::size_t pick = std::uniform_int_distribution<std::size_t>(0, m - 1)(rng);
        std::swap(values[pick], values[m - 1]);
        std::swap(origin[pick], origin[m - 1]);

        std::size_t boundary = 0;
        for (std::size_t i = 0; i + 1 < m; ++i) {
            ++trace.pivot_comparisons[origin[i]];
            ++trace.total_comparisons;
            if (values[i] < values[m - 1]) {
                std::swap(values[boundary], values[i]);
                std::swap(origin[boundary], origin[i]);
                ++boundary;
            }
        }
        std::swap(values[boundary], values[m - 1]);
        std::swap(origin[boundary], origin[m - 1]);

        // Recurse on the smaller side so stack depth stays logarithmic.
        const std::size_t right_start = boundary + 1;
        if (boundary < m - right_start) {
            traced_sort(values.first(boundary), origin.first(boundary), trace, rng);
            values = values.subspan(right_start);
            origin = origin.subspan(right_start);
        } else {
            traced_sort(values.subspan(right_start), origin.subspan(right_start), trace, rng);
            values = values.first(boundary);
            origin = origin.first(boundary);
        }
    }
}

// Three-way partition around pivot; returns [less_end, greater_begin).
template <typename T>
std::pair<std::size_t, std::size_t> partition3(std::span<T> values, const T& pivot) {
    std::size_t less_end = 0;
    std::size_t index = 0;
    std::size_t greater_begin = values.size();
    while (index < greater_begin) {
        if (values[index] < pivot) {
            std::swap(values[less_end], values[index]);
            ++less_end;
            ++index;
        } else if (pivot < values[index]) {
            --greater_begin;
            std::swap(values[index], values[greater_begin]);
        } else {
            ++index;
        }
    }
    return {less_end, greater_begin};
}

template <typename T, typename Rng>
void guarded(std::span<T> values, Rng& rng, int depth_left) {
    while (values.size() > 1) {
        if (depth_left == 0) {
            std::make_heap(values.begin(), values.end());
            std::sort_heap(values.begin(), values.end());
            return;
        }
        --depth_left;
        const std::size_t pick = std::uniform_int_distribution<std::size_t>(0, values.size() - 1)(rng);
        const T pivot = values[pick];
        const auto [less_end, greater_begin] = partition3(values, pivot);
        if (less_end < values.size() - greater_begin) {
            guarded(values.first(less_end), rng, depth_left);
            values = values.subspan(greater_begin);
        } else {
            guarded(values.subspan(greater_begin), rng, depth_left);
            values = values.first(less_end);
        }
    }
}

inline double logarithmic_constant() { return std::log(2.0) / std::log(4.0 / 3.0); }

}  // namespace analysis_detail

template <typename T, std::uniform_random_bit_generator Rng>
QuicksortTrace traced_quicksort(std::span<T> values, Rng& rng) {
    QuicksortTrace trace;
    trace.pivot_comparisons.assign(values.size(), 0);
    std::vector<std::uint32_t> origin(values.size());
    std::iota(origin.begin(), origin.end(), 0u);
    analysis_detail::traced_sort(values, std::span<std::uint32_t>(origin), trace, rng);
    return trace;
}

inline double element_bound(double n, int alpha) {
    return 3.0 * (alpha + 1 + analysis_detail::logarithmic_constant()) * std::log2(n);
}

inline double total_bound(double n, int alpha) { return n * element_bound(n, alpha); }

template <std::uniform_random_bit_generator Rng>
BoundReport measure_bound(std::size_t n, int trials, int alpha, Rng& rng) {
    std::vector<int> values(n);
    std::iota(values.begin(), values.end(), 0);
    const double n_real = static_cast<double>(n);

    BoundReport report;
    report.trials = trials;
    std::uint64_t sum = 0;
    for (int trial = 0; trial < trials; ++trial) {
        std::ranges::shuffle(values, rng);
        const QuicksortTrace trace = traced_quicksort<int>(values, rng);
        sum += trace.total_comparisons;
        const auto largest = *std::ranges::max_element(trace.pivot_comparisons);
        report.element_violations += static_cast<double>(largest) > element_bound(n_real, alpha);
        report.total_violations += static_cast<double>(trace.total_comparisons) > total_bound(n_real, alpha);
    }
    report.mean_total = static_cast<double>(sum) / trials;
    return report;
}

template <typename T, std::uniform_random_bit_generator Rng>
void guarded_sort(std::span<T> values, Rng& rng) {
    const int limit = values.size() < 2 ? 0 : 2 * std::bit_width(values.size() - 1);
    analysis_detail::guarded(values, rng, limit);
}
