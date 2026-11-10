#include <algorithm>
#include <cmath>
#include <vector>

#include "08_cache_sim.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Stopwatch;
using practice::growth_exponent;
using practice::cachesim::Cache;
using practice::cachesim::Memory;
using practice::cachesim::PlainMatrix;
using practice::cachesim::TracedMatrix;

namespace {

PlainMatrix<double> random_matrix(std::size_t n, Rng& rng) {
    PlainMatrix<double> m(n, n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            m.set(i, j, static_cast<double>(rng.uniform(-9, 9)));
        }
    }
    return m;
}

// Independent check: plain triple loop written directly against the underlying values.
PlainMatrix<double> expected_product(const PlainMatrix<double>& a, const PlainMatrix<double>& b) {
    const std::size_t n = a.rows();
    PlainMatrix<double> c(n, n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            double value = 0;
            for (std::size_t k = 0; k < n; ++k) {
                value += a.get(i, k) * b.get(k, j);
            }
            c.set(i, j, value);
        }
    }
    return c;
}

constexpr std::size_t kLine = 64;

// Simulated miss count of `algorithm` on an n×n instance for one cache geometry.
template <typename Algorithm>
std::uint64_t simulated_misses(std::size_t n, std::size_t capacity, Algorithm algorithm) {
    Cache cache(capacity, kLine, 8);
    Memory memory(cache);
    TracedMatrix<double> a(memory, n, n);
    TracedMatrix<double> b(memory, n, n);
    TracedMatrix<double> c(memory, n, n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            a.set(i, j, 1.0);
            b.set(i, j, 2.0);
        }
    }
    cache.reset_counters();
    algorithm(a, b, c);
    return cache.misses();
}

const auto kIjk = [](const auto& a, const auto& b, auto& c) { multiply_ijk(a, b, c); };
const auto kIkj = [](const auto& a, const auto& b, auto& c) { multiply_ikj(a, b, c); };
const auto kRecursive = [](const auto& a, const auto& b, auto& c) { multiply_recursive(a, b, c); };
auto tiled(std::size_t s) {
    return [s](const auto& a, const auto& b, auto& c) { multiply_tiled(a, b, c, s); };
}

template <typename Algorithm>
void check_correct(Algorithm algorithm, std::vector<std::size_t> sizes) {
    Rng rng;
    for (std::size_t n : sizes) {
        INFO("n=", n);
        const auto a = random_matrix(n, rng);
        const auto b = random_matrix(n, rng);
        PlainMatrix<double> c(n, n);
        algorithm(a, b, c);
        CHECK_EQ(c.values(), expected_product(a, b).values());
    }
}

}  // namespace

TEST(1, "ijk produces the matrix product") {
    check_correct(kIjk, {1, 2, 5, 16, 31});
}

TEST(1, "ikj produces the matrix product") {
    check_correct(kIkj, {1, 2, 5, 16, 31});
}

TEST(1, "results accumulate into the output") {
    PlainMatrix<double> a(2, 2), b(2, 2), c(2, 2, 10.0);
    a.set(0, 0, 1);
    b.set(0, 0, 3);
    multiply_ijk(a, b, c);
    CHECK_NEAR(c.get(0, 0), 13.0, 1e-12);
    CHECK_NEAR(c.get(1, 1), 10.0, 1e-12);
    PlainMatrix<double> d(2, 2, 10.0);
    multiply_ikj(a, b, d);
    CHECK_EQ(d.values(), c.values());
}

TEST(1, "the identity leaves a matrix unchanged") {
    Rng rng;
    const std::size_t n = 8;
    const auto m = random_matrix(n, rng);
    PlainMatrix<double> identity(n, n);
    for (std::size_t i = 0; i < n; ++i) {
        identity.set(i, i, 1.0);
    }
    PlainMatrix<double> c(n, n);
    multiply_ikj(m, identity, c);
    CHECK_EQ(c.values(), m.values());
}

TEST(1, "ijk misses on nearly every element access, ikj only per line") {
    const std::uint64_t ijk = simulated_misses(64, 8192, kIjk);
    const std::uint64_t ikj = simulated_misses(64, 8192, kIkj);
    const double cubic = std::pow(64.0, 3);
    INFO("ijk ", ijk, " ikj ", ikj);
    CHECK_GT(static_cast<double>(ijk), 0.5 * cubic);          // Θ(n³) misses, n > M/B
    CHECK_LT(static_cast<double>(ikj), 2.0 * cubic / 8.0);    // Θ(n³/B) with 8 doubles per line
    CHECK_GT(ijk, 5 * ikj);
}

TEST(2, "tiled multiplication is correct for awkward tile sizes") {
    Rng rng;
    const std::size_t n = 20;
    const auto a = random_matrix(n, rng);
    const auto b = random_matrix(n, rng);
    const auto want = expected_product(a, b);
    for (std::size_t s : {std::size_t{1}, std::size_t{3}, std::size_t{7}, n, 2 * n}) {
        INFO("s=", s);
        PlainMatrix<double> c(n, n);
        multiply_tiled(a, b, c, s);
        CHECK_EQ(c.values(), want.values());
    }
}

TEST(2, "a well-chosen tile beats plain loop interchange") {
    const std::uint64_t tiled_good = simulated_misses(128, 4096, tiled(8));
    const std::uint64_t interchange = simulated_misses(128, 4096, kIkj);
    INFO("tile 8 ", tiled_good, " ikj ", interchange);
    CHECK_LT(tiled_good * 10, interchange * 6);
}

TEST(2, "a badly chosen tile gives up most of the gain") {
    const std::uint64_t good = simulated_misses(128, 4096, tiled(8));
    const std::uint64_t bad = simulated_misses(128, 4096, tiled(64));
    INFO("tile 8 ", good, " tile 64 ", bad);
    CHECK_GT(bad, 2 * good);
}

TEST(3, "the recursive product is correct") {
    check_correct(kRecursive, {1, 2, 4, 8, 32});
}

TEST(3, "recursion matches the tile tuned for the cache") {
    const std::uint64_t recursive = simulated_misses(128, 4096, kRecursive);
    const std::uint64_t tuned = simulated_misses(128, 4096, tiled(8));
    INFO("recursive ", recursive, " tuned tile ", tuned);
    CHECK_LT(recursive * 10, tuned * 14);
}

TEST(3, "tuning for a bigger cache loses badly but recursion needs no tuning") {
    // s = 24 suits a 32 KiB cache; on 4 KiB the same tiling is near-useless.
    const std::uint64_t wrongly_tuned = simulated_misses(128, 4096, tiled(24));
    const std::uint64_t recursive = simulated_misses(128, 4096, kRecursive);
    INFO("mis-tuned tile ", wrongly_tuned, " recursive ", recursive);
    CHECK_GT(wrongly_tuned * 10, recursive * 25);
}

TEST(3, "misses scale with the square root of the cache size") {
    const std::uint64_t small_cache = simulated_misses(128, 4096, kRecursive);
    const std::uint64_t large_cache = simulated_misses(128, 16384, kRecursive);
    INFO("4 KiB ", small_cache, " 16 KiB ", large_cache);
    // Theory: Q(n) = Θ(n³ / (B √M)), so quadrupling M halves the misses.
    const double ratio = static_cast<double>(small_cache) / static_cast<double>(large_cache);
    CHECK_GT(ratio, 1.4);
    CHECK_LT(ratio, 2.8);
}

TEST(3, "misses grow like n cubed once the matrices dwarf the cache") {
    const std::uint64_t q64 = simulated_misses(64, 4096, kRecursive);
    const std::uint64_t q128 = simulated_misses(128, 4096, kRecursive);
    const double exponent = growth_exponent(static_cast<double>(q64), static_cast<double>(q128),
                                            64.0, 128.0);
    INFO("exponent ", exponent);
    CHECK_GT(exponent, 2.6);
    CHECK_LT(exponent, 3.4);
}

namespace {

template <typename Fn>
double best_of(int rounds, Fn fn) {
    double best = 1e18;
    for (int i = 0; i < rounds; ++i) {
        const Stopwatch watch;
        fn();
        best = std::min(best, watch.elapsed_ms());
    }
    return best;
}

}  // namespace

PERF(1, "loop interchange wins by a large margin on real hardware", 6000) {
    Rng rng;
    const std::size_t n = 512;
    const auto a = random_matrix(n, rng);
    const auto b = random_matrix(n, rng);
    const double ijk = best_of(3, [&] {
        PlainMatrix<double> c(n, n);
        multiply_ijk(a, b, c);
    });
    const double ikj = best_of(3, [&] {
        PlainMatrix<double> c(n, n);
        multiply_ikj(a, b, c);
    });
    INFO("ijk ", ijk, " ms, ikj ", ikj, " ms");
    CHECK_GT(ijk, 3.0 * ikj);
}
