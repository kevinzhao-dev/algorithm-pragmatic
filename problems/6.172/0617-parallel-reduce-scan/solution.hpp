#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

#include "01_exec.hpp"
#include "practice.hpp"

// Stage 1
template <typename Exec, typename T, typename Op>
T p_reduce(Exec& exec, std::span<const T> x, Op op) {
    PRACTICE_TODO();
}

// Stage 2
template <typename Exec, typename T, typename Op>
std::vector<T> p_scan_1(Exec& exec, std::span<const T> x, Op op) {
    PRACTICE_TODO();
}

// Stage 3
template <typename Exec, typename T, typename Op>
std::vector<T> p_scan_2(Exec& exec, std::span<const T> x, Op op) {
    PRACTICE_TODO();
}

// Stage 4
template <typename Exec, typename T, typename Op>
std::vector<T> p_scan_3(Exec& exec, std::span<const T> x, Op op) {
    PRACTICE_TODO();
}

// Stage 5
template <typename Exec, typename T, typename Pred>
std::vector<T> parallel_filter(Exec& exec, std::span<const T> x, Pred pred) {
    PRACTICE_TODO();
}
