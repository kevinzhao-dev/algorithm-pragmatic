#pragma once

#include <cstddef>
#include <span>

#include "01_exec.hpp"
#include "practice.hpp"

// Stage 1
template <typename Exec>
void transpose_square(Exec& exec, std::span<int> grid, std::size_t n, std::size_t cutoff = 1) {
    PRACTICE_TODO();
}

// Stage 2
template <typename Exec>
void transpose_copy(Exec& exec, std::span<const int> src, std::size_t rows, std::size_t cols,
                    std::span<int> dst, std::size_t cutoff = 1) {
    PRACTICE_TODO();
}
