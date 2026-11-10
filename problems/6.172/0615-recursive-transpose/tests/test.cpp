#include <algorithm>
#include <bit>
#include <numeric>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::vector<int> iota_grid(std::size_t n) {
    std::vector<int> grid(n * n);
    std::iota(grid.begin(), grid.end(), 0);
    return grid;
}

std::vector<int> transposed(const std::vector<int>& grid, std::size_t n) {
    std::vector<int> out(n * n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            out[j * n + i] = grid[i * n + j];
        }
    }
    return out;
}

std::size_t ceil_lg(std::size_t n) { return n <= 1 ? 0 : static_cast<std::size_t>(std::bit_width(n - 1)); }

}  // namespace

TEST(1, "known small transposes") {
    exec::Meter meter;
    std::vector<int> two = {1, 2, 3, 4};
    transpose_square(meter, std::span<int>(two), 2);
    CHECK_EQ(two, std::vector<int>({1, 3, 2, 4}));
    std::vector<int> three = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    transpose_square(meter, std::span<int>(three), 3);
    CHECK_EQ(three, std::vector<int>({1, 4, 7, 2, 5, 8, 3, 6, 9}));
}

TEST(1, "the smallest grids are no-ops") {
    exec::Meter meter;
    std::vector<int> one = {5};
    transpose_square(meter, std::span<int>(one), 1);
    CHECK_EQ(one, std::vector<int>{5});
    std::vector<int> none;
    transpose_square(meter, std::span<int>(none), 0);
    CHECK(none.empty());
}

TEST(1, "agrees with the plain definition for every size and cutoff") {
    exec::Meter meter;
    for (std::size_t n = 0; n <= 80; ++n) {
        for (const std::size_t cutoff : {1, 2, 3, 16, 100}) {
            std::vector<int> grid = iota_grid(n);
            const std::vector<int> expected = transposed(grid, n);
            transpose_square(meter, std::span<int>(grid), n, cutoff);
            INFO("n ", n, " cutoff ", cutoff);
            REQUIRE_EQ(grid, expected);
        }
    }
}

TEST(1, "work is exactly the swaps plus the forks") {
    for (const std::size_t n : {2, 3, 16, 17, 64, 200}) {
        std::vector<int> grid = iota_grid(n);
        exec::Meter meter;
        transpose_square(meter, std::span<int>(grid), n);
        INFO("n ", n);
        CHECK_EQ(meter.work() - meter.spawns(), static_cast<std::int64_t>(n * (n - 1) / 2));
    }
}

TEST(1, "span is logarithmic") {
    for (const std::size_t n : {8, 64, 128, 1024, 4096}) {
        std::vector<int> grid = iota_grid(n);
        exec::Meter meter;
        transpose_square(meter, std::span<int>(grid), n);
        INFO("n ", n, " span ", meter.span());
        CHECK_LE(meter.span(), static_cast<std::int64_t>(8 * ceil_lg(n) + 16));
        CHECK_GT(meter.parallelism(), static_cast<double>(n) / 2);
    }
}

TEST(1, "no race on the real pool") {
    exec::ForkJoin pool;
    for (const unsigned workers : {1U, 4U, 8U}) {
        practice::set_workers(workers);
        for (const std::size_t n : {1, 2, 57, 128, 500}) {
            std::vector<int> grid = iota_grid(n);
            const std::vector<int> expected = transposed(grid, n);
            transpose_square(pool, std::span<int>(grid), n, 4);
            INFO("workers ", workers, " n ", n);
            REQUIRE_EQ(grid, expected);
        }
    }
}

TEST(2, "rectangles land in the right place") {
    exec::Meter meter;
    const std::vector<int> src = {1, 2, 3, 4, 5, 6};  // 2 rows, 3 columns
    std::vector<int> dst(6, -1);
    transpose_copy(meter, std::span<const int>(src), 2, 3, std::span<int>(dst));
    CHECK_EQ(dst, std::vector<int>({1, 4, 2, 5, 3, 6}));
}

TEST(2, "degenerate shapes") {
    exec::Meter meter;
    const std::vector<int> column = {1, 2, 3};
    std::vector<int> row(3);
    transpose_copy(meter, std::span<const int>(column), 3, 1, std::span<int>(row));
    CHECK_EQ(row, std::vector<int>({1, 2, 3}));
    std::vector<int> nothing;
    transpose_copy(meter, std::span<const int>(), 0, 7, std::span<int>(nothing));
    CHECK(nothing.empty());
}

TEST(2, "random rectangles and cutoffs agree with the definition") {
    Rng rng;
    exec::Meter meter;
    for (int round = 0; round < 400; ++round) {
        const auto rows = static_cast<std::size_t>(rng.uniform(0, 40));
        const auto cols = static_cast<std::size_t>(rng.uniform(0, 40));
        const auto cutoff = static_cast<std::size_t>(rng.uniform(1, 64));
        const std::vector<int> src = rng.ints(rows * cols, -1000, 1000);
        std::vector<int> dst(rows * cols, 0);
        transpose_copy(meter, std::span<const int>(src), rows, cols, std::span<int>(dst), cutoff);
        std::vector<int> expected(rows * cols);
        for (std::size_t r = 0; r < rows; ++r) {
            for (std::size_t c = 0; c < cols; ++c) {
                expected[c * rows + r] = src[r * cols + c];
            }
        }
        INFO("rows ", rows, " cols ", cols, " cutoff ", cutoff);
        REQUIRE_EQ(dst, expected);
    }
}

TEST(2, "work and span follow the recursion") {
    for (const auto& [rows, cols] : {std::pair<std::size_t, std::size_t>{4, 4}, {7, 3}, {100, 1}, {1000, 8}, {512, 1024}}) {
        const std::vector<int> src(rows * cols, 1);
        std::vector<int> dst(rows * cols);
        exec::Meter meter;
        transpose_copy(meter, std::span<const int>(src), rows, cols, std::span<int>(dst));
        INFO("rows ", rows, " cols ", cols);
        CHECK_EQ(meter.work() - meter.spawns(), static_cast<std::int64_t>(rows * cols));
        CHECK_LE(meter.span(), static_cast<std::int64_t>(4 * (ceil_lg(rows) + ceil_lg(cols)) + 16));
    }
}

TEST(2, "no race on the real pool") {
    Rng rng;
    exec::ForkJoin pool;
    practice::set_workers(8);
    const std::size_t rows = 300;
    const std::size_t cols = 170;
    const std::vector<int> src = rng.ints(rows * cols, 0, 9);
    std::vector<int> dst(rows * cols);
    transpose_copy(pool, std::span<const int>(src), rows, cols, std::span<int>(dst), 16);
    std::vector<int> expected(rows * cols);
    for (std::size_t r = 0; r < rows; ++r) {
        for (std::size_t c = 0; c < cols; ++c) {
            expected[c * rows + r] = src[r * cols + c];
        }
    }
    CHECK_EQ(dst, expected);
}
