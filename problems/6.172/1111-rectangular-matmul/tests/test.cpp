#include <cmath>
#include <vector>

#include "08_cache_sim.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::cachesim::Cache;
using practice::cachesim::Memory;
using practice::cachesim::PlainMatrix;
using practice::cachesim::TracedMatrix;

namespace {

PlainMatrix<double> random_matrix(std::size_t rows, std::size_t cols, Rng& rng) {
    PlainMatrix<double> m(rows, cols);
    for (std::size_t i = 0; i < rows; ++i) {
        for (std::size_t j = 0; j < cols; ++j) {
            m.set(i, j, static_cast<double>(rng.uniform(-5, 5)));
        }
    }
    return m;
}

PlainMatrix<double> expected_product(const PlainMatrix<double>& a, const PlainMatrix<double>& b) {
    PlainMatrix<double> c(a.rows(), b.cols());
    for (std::size_t i = 0; i < a.rows(); ++i) {
        for (std::size_t j = 0; j < b.cols(); ++j) {
            double value = 0;
            for (std::size_t k = 0; k < a.cols(); ++k) {
                value += a.get(i, k) * b.get(k, j);
            }
            c.set(i, j, value);
        }
    }
    return c;
}

struct Shape {
    std::size_t m, n, r;
};

// A fully associative 4 KiB cache with 64-byte lines: M = 512 elements, B = 8 elements.
constexpr std::size_t kCapacity = 4096;
constexpr std::size_t kLineBytes = 64;
const CacheGeometry kGeometry{512.0, 8.0};

template <typename Algorithm>
std::uint64_t simulated_misses(const Shape& shape, Algorithm algorithm) {
    Cache cache(kCapacity, kLineBytes, kCapacity / kLineBytes);
    Memory memory(cache);
    TracedMatrix<double> a(memory, shape.m, shape.n);
    TracedMatrix<double> b(memory, shape.n, shape.r);
    TracedMatrix<double> c(memory, shape.m, shape.r);
    for (std::size_t i = 0; i < shape.m; ++i) {
        for (std::size_t j = 0; j < shape.n; ++j) {
            a.set(i, j, 1.0);
        }
    }
    for (std::size_t i = 0; i < shape.n; ++i) {
        for (std::size_t j = 0; j < shape.r; ++j) {
            b.set(i, j, 2.0);
        }
    }
    cache.reset_counters();
    algorithm(a, b, c);
    return cache.misses();
}

const auto kNaive = [](const auto& a, const auto& b, auto& c) { multiply_rect_naive(a, b, c); };
const auto kRecursive = [](const auto& a, const auto& b, auto& c) {
    multiply_rect_recursive(a, b, c);
};

template <typename Algorithm>
void check_shapes(Algorithm algorithm) {
    Rng rng;
    const std::vector<Shape> shapes{{1, 1, 1},  {1, 7, 1}, {5, 1, 4},  {1, 1, 9}, {4, 6, 8},
                                    {7, 13, 5}, {16, 2, 9}, {3, 50, 3}, {9, 9, 9}};
    for (const Shape& shape : shapes) {
        INFO("shape ", shape.m, "x", shape.n, "x", shape.r);
        const auto a = random_matrix(shape.m, shape.n, rng);
        const auto b = random_matrix(shape.n, shape.r, rng);
        PlainMatrix<double> c(shape.m, shape.r);
        algorithm(a, b, c);
        CHECK_EQ(c.values(), expected_product(a, b).values());
    }
}

}  // namespace

TEST(1, "naive product is correct across shapes") {
    check_shapes(kNaive);
}

TEST(1, "naive product accumulates into the output") {
    PlainMatrix<double> a(1, 2), b(2, 1), c(1, 1, 3.0);
    a.set(0, 0, 2);
    a.set(0, 1, 5);
    b.set(0, 0, 1);
    b.set(1, 0, 4);
    multiply_rect_naive(a, b, c);
    CHECK_NEAR(c.get(0, 0), 3.0 + 2.0 + 20.0, 1e-12);
}

TEST(2, "recursive product is correct across shapes") {
    check_shapes(kRecursive);
}

TEST(2, "recursive product accumulates into the output") {
    PlainMatrix<double> a(2, 3, 1.0), b(3, 2, 1.0), c(2, 2, 4.0);
    multiply_rect_recursive(a, b, c);
    CHECK_NEAR(c.get(0, 0), 7.0, 1e-12);
    CHECK_NEAR(c.get(1, 1), 7.0, 1e-12);
}

TEST(3, "naive is predicted within a factor of two when n exceeds M/B") {
    for (const Shape shape : std::vector<Shape>{{8, 512, 8}, {4, 1024, 16}, {16, 256, 32}}) {
        INFO("shape ", shape.m, "x", shape.n, "x", shape.r);
        const auto actual = static_cast<double>(simulated_misses(shape, kNaive));
        const double predicted = predict_naive_misses(static_cast<double>(shape.m),
                                                      static_cast<double>(shape.n),
                                                      static_cast<double>(shape.r), kGeometry);
        CHECK_GT(predicted * 2, actual);
        CHECK_GT(actual * 2, predicted);
    }
}

TEST(3, "naive is predicted within a factor of two when M/r < n < M/B") {
    for (const Shape shape : std::vector<Shape>{{24, 48, 64}, {16, 32, 128}, {128, 32, 64}}) {
        INFO("shape ", shape.m, "x", shape.n, "x", shape.r);
        const auto actual = static_cast<double>(simulated_misses(shape, kNaive));
        const double predicted = predict_naive_misses(static_cast<double>(shape.m),
                                                      static_cast<double>(shape.n),
                                                      static_cast<double>(shape.r), kGeometry);
        CHECK_GT(predicted * 2, actual);
        CHECK_GT(actual * 2, predicted);
    }
}

TEST(3, "the recursive version is predicted within a factor of four") {
    for (const Shape shape : std::vector<Shape>{{32, 512, 32}, {24, 48, 64}, {128, 32, 64}, {64, 128, 32}}) {
        INFO("shape ", shape.m, "x", shape.n, "x", shape.r);
        const auto actual = static_cast<double>(simulated_misses(shape, kRecursive));
        const double predicted = predict_recursive_misses(
            static_cast<double>(shape.m), static_cast<double>(shape.n),
            static_cast<double>(shape.r), kGeometry);
        CHECK_GT(predicted * 4, actual);
        CHECK_GT(actual * 4, predicted);
    }
}

TEST(3, "recursion avoids the scan cliff when the column does not fit") {
    const Shape shape{32, 512, 32};
    const std::uint64_t naive = simulated_misses(shape, kNaive);
    const std::uint64_t recursive = simulated_misses(shape, kRecursive);
    INFO("naive ", naive, " recursive ", recursive);
    CHECK_GT(naive, 10 * recursive);
}

TEST(3, "recursion is still better in the middle regime") {
    const Shape shape{128, 32, 64};
    const std::uint64_t naive = simulated_misses(shape, kNaive);
    const std::uint64_t recursive = simulated_misses(shape, kRecursive);
    INFO("naive ", naive, " recursive ", recursive);
    CHECK_GT(naive, 2 * recursive);
}
