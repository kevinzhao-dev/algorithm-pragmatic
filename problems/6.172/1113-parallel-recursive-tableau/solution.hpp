#pragma once

#include <cstddef>
#include <cstdint>

#include "forkjoin.hpp"
#include "practice.hpp"

// Stage 1. Arr has exactly 2n - 1 cells, exposes size(), get(k), set(k, v).
template <typename Arr, typename F>
double recursive_tableau(Arr& a, std::size_t n, double init, F f) {
    PRACTICE_TODO();
}

// Stage 2
template <typename Arr, typename F>
double parallel_tableau(Arr& a, std::size_t n, double init, F f) {
    PRACTICE_TODO();
}

// Stage 3
struct WorkSpan {
    std::uint64_t work = 0;
    std::uint64_t span = 0;
};

inline WorkSpan tableau_work_span(std::size_t n, std::size_t k) { PRACTICE_TODO(); }
