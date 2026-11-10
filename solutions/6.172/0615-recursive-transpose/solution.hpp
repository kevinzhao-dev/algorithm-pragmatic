#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <utility>

#include "01_exec.hpp"

namespace detail_0615 {

// Swaps element (r, c) with (c, r) for r in [r0, r0+h), c in [c0, c0+w); the two blocks are disjoint.
template <typename Exec>
void swap_blocks(Exec& exec, std::span<int> a, std::size_t n, std::size_t r0, std::size_t c0,
                 std::size_t h, std::size_t w, std::size_t cutoff) {
    if (h == 0 || w == 0) {
        return;
    }
    if (h * w <= std::max<std::size_t>(cutoff * cutoff, 1)) {
        for (std::size_t r = r0; r < r0 + h; ++r) {
            for (std::size_t c = c0; c < c0 + w; ++c) {
                exec.charge();
                std::swap(a[r * n + c], a[c * n + r]);
            }
        }
        return;
    }
    if (h >= w) {
        const std::size_t half = h / 2;
        exec.fork2([&] { swap_blocks(exec, a, n, r0, c0, half, w, cutoff); },
                   [&] { swap_blocks(exec, a, n, r0 + half, c0, h - half, w, cutoff); });
    } else {
        const std::size_t half = w / 2;
        exec.fork2([&] { swap_blocks(exec, a, n, r0, c0, h, half, cutoff); },
                   [&] { swap_blocks(exec, a, n, r0, c0 + half, h, w - half, cutoff); });
    }
}

template <typename Exec>
void transpose_diagonal(Exec& exec, std::span<int> a, std::size_t n, std::size_t lo, std::size_t size,
                        std::size_t cutoff) {
    if (size <= std::max<std::size_t>(cutoff, 1)) {
        for (std::size_t i = lo; i < lo + size; ++i) {
            for (std::size_t j = i + 1; j < lo + size; ++j) {
                exec.charge();
                std::swap(a[i * n + j], a[j * n + i]);
            }
        }
        return;
    }
    const std::size_t half = size / 2;
    // A11, A22 and the A12/A21 exchange touch pairwise disjoint cells, so all three may overlap.
    exec.fork2(
        [&] {
            exec.fork2([&] { transpose_diagonal(exec, a, n, lo, half, cutoff); },
                       [&] { transpose_diagonal(exec, a, n, lo + half, size - half, cutoff); });
        },
        [&] { swap_blocks(exec, a, n, lo + half, lo, size - half, half, cutoff); });
}

template <typename Exec>
void copy_blocks(Exec& exec, std::span<const int> src, std::size_t rows, std::size_t cols,
                 std::span<int> dst, std::size_t r0, std::size_t c0, std::size_t h, std::size_t w,
                 std::size_t cutoff) {
    if (h == 0 || w == 0) {
        return;
    }
    if (h * w <= std::max<std::size_t>(cutoff, 1)) {
        for (std::size_t r = r0; r < r0 + h; ++r) {
            for (std::size_t c = c0; c < c0 + w; ++c) {
                exec.charge();
                dst[c * rows + r] = src[r * cols + c];
            }
        }
        return;
    }
    if (h >= w) {
        const std::size_t half = h / 2;
        exec.fork2([&] { copy_blocks(exec, src, rows, cols, dst, r0, c0, half, w, cutoff); },
                   [&] { copy_blocks(exec, src, rows, cols, dst, r0 + half, c0, h - half, w, cutoff); });
    } else {
        const std::size_t half = w / 2;
        exec.fork2([&] { copy_blocks(exec, src, rows, cols, dst, r0, c0, h, half, cutoff); },
                   [&] { copy_blocks(exec, src, rows, cols, dst, r0, c0 + half, h, w - half, cutoff); });
    }
}

}  // namespace detail_0615

template <typename Exec>
void transpose_square(Exec& exec, std::span<int> grid, std::size_t n, std::size_t cutoff = 1) {
    detail_0615::transpose_diagonal(exec, grid, n, 0, n, cutoff);
}

template <typename Exec>
void transpose_copy(Exec& exec, std::span<const int> src, std::size_t rows, std::size_t cols,
                    std::span<int> dst, std::size_t cutoff = 1) {
    detail_0615::copy_blocks(exec, src, rows, cols, dst, 0, 0, rows, cols, cutoff);
}
