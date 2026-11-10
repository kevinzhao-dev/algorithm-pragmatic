#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <random>
#include <vector>

#include "01_bench.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

#define NI [[gnu::noinline]]
#define FORCE_SCALAR _Pragma("clang loop vectorize(disable) interleave(disable)")

namespace {

NI void scalar_add_bytes(std::uint8_t* a, const std::uint8_t* b, std::size_t n) {
    FORCE_SCALAR for (std::size_t i = 0; i < n; ++i) {
        a[i] = static_cast<std::uint8_t>(a[i] + b[i]);
    }
}

NI void scalar_max_bytes(std::uint8_t* a, const std::uint8_t* b, std::size_t n) {
    FORCE_SCALAR for (std::size_t i = 0; i < n; ++i) {
        a[i] = b[i] > a[i] ? b[i] : a[i];
    }
}

NI double scalar_sum(const double* x, std::size_t n) {
    double total = 0;
    FORCE_SCALAR for (std::size_t i = 0; i < n; ++i) {
        total += x[i];
    }
    return total;
}

NI float scalar_dot(const float* a, const float* b, std::size_t n) {
    float total = 0;
    FORCE_SCALAR for (std::size_t i = 0; i < n; ++i) {
        total += a[i] * b[i];
    }
    return total;
}

template <typename T>
NI void scalar_add_lanes(T* out, const T* a, const T* b, std::size_t n) {
    FORCE_SCALAR for (std::size_t i = 0; i < n; ++i) {
        out[i] = static_cast<T>(a[i] + b[i]);
    }
}

NI void scalar_row_sums(const int* m, std::size_t rows, std::size_t cols, int* out) {
    FORCE_SCALAR for (std::size_t row = 0; row < rows; ++row) {
        int total = 0;
        FORCE_SCALAR for (std::size_t col = 0; col < cols; ++col) {
            total += m[row * cols + col];
        }
        out[row] = total;
    }
}

NI std::optional<std::size_t> scalar_best_match(const float* query, const float* matrix,
                                                std::size_t rows, std::size_t dim) {
    std::size_t best_row = 0;
    float best_score = -std::numeric_limits<float>::infinity();
    FORCE_SCALAR for (std::size_t row = 0; row < rows; ++row) {
        float score = 0;
        FORCE_SCALAR for (std::size_t j = 0; j < dim; ++j) {
            score += query[j] * matrix[row * dim + j];
        }
        if (score > best_score) {
            best_score = score;
            best_row = row;
        }
    }
    return best_row;
}

struct AosParticle {
    float x, y, z, vx, vy, vz;
};

NI void aos_integrate(AosParticle* p, std::size_t n, float dt) {
    for (std::size_t i = 0; i < n; ++i) {
        p[i].x += p[i].vx * dt;
        p[i].y += p[i].vy * dt;
        p[i].z += p[i].vz * dt;
    }
}

std::vector<std::uint8_t> random_bytes(std::size_t n, std::uint64_t seed) {
    std::mt19937_64 engine(seed);
    std::vector<std::uint8_t> bytes(n);
    for (auto& byte : bytes) {
        byte = static_cast<std::uint8_t>(engine());
    }
    return bytes;
}

template <typename T>
std::vector<T> random_values(std::size_t n, std::uint64_t seed) {
    std::mt19937_64 engine(seed);
    std::vector<T> values(n);
    for (auto& value : values) {
        value = static_cast<T>(engine());
    }
    return values;
}

}  // namespace

TEST(1, "add_bytes adds with wraparound") {
    std::vector<std::uint8_t> a = {1, 2, 250, 255};
    const std::vector<std::uint8_t> b = {10, 20, 10, 1};
    add_bytes(a.data(), b.data(), a.size());
    CHECK_EQ(a, std::vector<std::uint8_t>({11, 22, 4, 0}));
    add_bytes(a.data(), b.data(), 0);
    CHECK_EQ(a[0], 11);
}

TEST(1, "add_bytes matches a model on random disjoint inputs") {
    Rng rng;
    for (int round = 0; round < 40; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(0, 5000));
        std::vector<std::uint8_t> a = random_bytes(n, 11 + static_cast<std::uint64_t>(round));
        const std::vector<std::uint8_t> b = random_bytes(n, 99 + static_cast<std::uint64_t>(round));
        std::vector<std::uint8_t> expected = a;
        for (std::size_t i = 0; i < n; ++i) {
            expected[i] = static_cast<std::uint8_t>(expected[i] + b[i]);
        }
        add_bytes(a.data(), b.data(), n);
        INFO("n ", n);
        REQUIRE_EQ(a, expected);
    }
}

PERF(1, "overlapping ranges still behave like the sequential loop", 2000) {
    // a overlaps b from the right: every a[i] reads a value the previous iteration just wrote.
    for (const std::size_t n : {std::size_t{1000}, std::size_t{100'000}}) {
        std::vector<std::uint8_t> buffer = random_bytes(n + 1, 3);
        std::vector<std::uint8_t> expected = buffer;
        for (std::size_t i = 0; i < n; ++i) {
            expected[i + 1] = static_cast<std::uint8_t>(expected[i + 1] + expected[i]);
        }
        add_bytes(buffer.data() + 1, buffer.data(), n);
        INFO("n ", n);
        CHECK_EQ(buffer, expected);
    }
}

PERF(1, "add_bytes vectorizes", 5000) {
    const std::size_t n = 1 << 16;
    std::vector<std::uint8_t> a = random_bytes(n, 1);
    const std::vector<std::uint8_t> b = random_bytes(n, 2);
    const double ratio = bench::speedup(
        5, [&] { for (int r = 0; r < 2000; ++r) scalar_add_bytes(a.data(), b.data(), n); },
        [&] { for (int r = 0; r < 2000; ++r) add_bytes(a.data(), b.data(), n); });
    INFO("speedup ", ratio);
    CHECK_GE(ratio, 6.0);
}

TEST(2, "max_bytes takes the larger byte") {
    std::vector<std::uint8_t> a = {1, 9, 200, 7};
    const std::vector<std::uint8_t> b = {5, 2, 200, 255};
    max_bytes(a.data(), b.data(), a.size());
    CHECK_EQ(a, std::vector<std::uint8_t>({5, 9, 200, 255}));
}

TEST(2, "max_bytes matches a model on random input") {
    Rng rng;
    for (int round = 0; round < 40; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(0, 3000));
        std::vector<std::uint8_t> a = random_bytes(n, 5 + static_cast<std::uint64_t>(round));
        const std::vector<std::uint8_t> b = random_bytes(n, 7 + static_cast<std::uint64_t>(round));
        std::vector<std::uint8_t> expected = a;
        for (std::size_t i = 0; i < n; ++i) {
            expected[i] = std::max(expected[i], b[i]);
        }
        max_bytes(a.data(), b.data(), n);
        REQUIRE_EQ(a, expected);
    }
}

PERF(2, "max_bytes vectorizes", 5000) {
    const std::size_t n = 1 << 16;
    std::vector<std::uint8_t> a = random_bytes(n, 1);
    const std::vector<std::uint8_t> b = random_bytes(n, 2);
    const double ratio = bench::speedup(
        5, [&] { for (int r = 0; r < 2000; ++r) scalar_max_bytes(a.data(), b.data(), n); },
        [&] { for (int r = 0; r < 2000; ++r) max_bytes(a.data(), b.data(), n); });
    INFO("speedup ", ratio);
    CHECK_GE(ratio, 6.0);
}

TEST(3, "sum_doubles is exact when no rounding can occur") {
    const std::vector<double> values = {1.0, 2.0, 4.0, 8.0, 1048576.0, 0.25};
    CHECK_EQ(sum_doubles(values.data(), values.size()), 1048591.25);
    CHECK_EQ(sum_doubles(values.data(), 0), 0.0);
}

TEST(3, "reassociated sums stay within rounding error of the true sum") {
    Rng rng;
    for (int round = 0; round < 40; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 50'000));
        std::vector<double> values(n);
        double absolute_sum = 0;
        for (double& value : values) {
            value = rng.real() * 1e6 - 5e5;
            absolute_sum += std::abs(value);
        }
        // Kahan compensated sum as the reference.
        double kahan = 0;
        double carry = 0;
        for (const double value : values) {
            const double y = value - carry;
            const double t = kahan + y;
            carry = (t - kahan) - y;
            kahan = t;
        }
        const double epsilon = std::numeric_limits<double>::epsilon();
        const double got = sum_doubles(values.data(), n);
        INFO("n ", n, " got ", got, " kahan ", kahan);
        CHECK_LE(std::abs(got - kahan), static_cast<double>(n) * epsilon * absolute_sum + 1e-6);
    }
}

TEST(3, "dot_floats computes inner products") {
    const std::vector<float> a = {1, 2, 3};
    const std::vector<float> b = {4, -5, 6};
    CHECK_EQ(dot_floats(a.data(), b.data(), 3), 12.0F);
    CHECK_EQ(dot_floats(a.data(), b.data(), 0), 0.0F);
}

TEST(3, "dot_floats agrees with a scalar loop on small integers") {
    Rng rng;
    for (int round = 0; round < 100; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(0, 300));
        std::vector<float> a(n);
        std::vector<float> b(n);
        float expected = 0;
        for (std::size_t i = 0; i < n; ++i) {
            a[i] = static_cast<float>(rng.uniform(-9, 9));
            b[i] = static_cast<float>(rng.uniform(-9, 9));
            expected += a[i] * b[i];
        }
        CHECK_EQ(dot_floats(a.data(), b.data(), n), expected);
    }
}

PERF(3, "floating-point reductions reassociate into lanes", 6000) {
    const std::size_t n = 1 << 16;
    Rng rng;
    std::vector<double> doubles(n);
    std::vector<float> a(n);
    std::vector<float> b(n);
    for (std::size_t i = 0; i < n; ++i) {
        doubles[i] = rng.real();
        a[i] = static_cast<float>(rng.real());
        b[i] = static_cast<float>(rng.real());
    }
    {
        const double ratio = bench::speedup(
            5,
            [&] { for (int r = 0; r < 800; ++r) bench::do_not_optimize(scalar_sum(doubles.data(), n)); },
            [&] { for (int r = 0; r < 800; ++r) bench::do_not_optimize(sum_doubles(doubles.data(), n)); });
        INFO("sum speedup ", ratio);
        CHECK_GE(ratio, 3.0);
    }
    {
        const double ratio = bench::speedup(
            5,
            [&] { for (int r = 0; r < 800; ++r) bench::do_not_optimize(scalar_dot(a.data(), b.data(), n)); },
            [&] { for (int r = 0; r < 800; ++r) bench::do_not_optimize(dot_floats(a.data(), b.data(), n)); });
        INFO("dot speedup ", ratio);
        CHECK_GE(ratio, 4.0);
    }
}

TEST(4, "add_lanes is correct for every width") {
    {
        const std::vector<std::uint8_t> a = {250, 1};
        const std::vector<std::uint8_t> b = {10, 2};
        std::vector<std::uint8_t> out(2);
        add_lanes(out.data(), a.data(), b.data(), 2);
        CHECK_EQ(out, std::vector<std::uint8_t>({4, 3}));
    }
    {
        const std::vector<std::uint64_t> a = {~std::uint64_t{0}, 5};
        const std::vector<std::uint64_t> b = {2, 7};
        std::vector<std::uint64_t> out(2);
        add_lanes(out.data(), a.data(), b.data(), 2);
        CHECK_EQ(out, std::vector<std::uint64_t>({1, 12}));
    }
    {
        const std::vector<std::uint16_t> a = {65535};
        const std::vector<std::uint16_t> b = {1};
        std::vector<std::uint16_t> out(1);
        add_lanes(out.data(), a.data(), b.data(), 1);
        CHECK_EQ(out, std::vector<std::uint16_t>{0});
        const std::vector<std::uint32_t> c = {4'000'000'000};
        const std::vector<std::uint32_t> d = {1'000'000'000};
        std::vector<std::uint32_t> out32(1);
        add_lanes(out32.data(), c.data(), d.data(), 1);
        CHECK_EQ(out32[0], 705'032'704U);
    }
}

namespace {

template <typename T>
double lane_speedup() {
    const std::size_t n = 4096;
    const std::vector<T> a = random_values<T>(n, 21);
    const std::vector<T> b = random_values<T>(n, 22);
    std::vector<T> out(n);
    const int rounds = 20'000;
    return bench::speedup(
        5,
        [&] {
            for (int r = 0; r < rounds; ++r) {
                scalar_add_lanes<T>(out.data(), a.data(), b.data(), n);
                bench::do_not_optimize(out.data());
            }
        },
        [&] {
            for (int r = 0; r < rounds; ++r) {
                add_lanes<T>(out.data(), a.data(), b.data(), n);
                bench::do_not_optimize(out.data());
            }
        });
}

}  // namespace

PERF(4, "narrower elements mean more lanes", 12000) {
    const double u8 = lane_speedup<std::uint8_t>();
    const double u16 = lane_speedup<std::uint16_t>();
    const double u32 = lane_speedup<std::uint32_t>();
    const double u64 = lane_speedup<std::uint64_t>();
    INFO("u8 ", u8, " u16 ", u16, " u32 ", u32, " u64 ", u64);
    CHECK_GE(u8, 8.0);
    CHECK_GE(u16, 4.0);
    CHECK_GE(u32, 2.0);
    CHECK_GE(u64, 1.4);
}

TEST(5, "a new store is zeroed and exposes its fields") {
    ParticleStore store(3);
    CHECK_EQ(store.size(), 3);
    CHECK_EQ(store.x().size(), 3);
    CHECK_EQ(store.vz()[2], 0.0F);
    store.x()[1] = 5;
    store.vx()[1] = 2;
    store.integrate(0.5F);
    CHECK_EQ(store.x()[1], 6.0F);
    CHECK_EQ(store.y()[1], 0.0F);
}

TEST(5, "integrate updates every component of every particle") {
    Rng rng;
    const std::size_t n = 500;
    ParticleStore store(n);
    std::vector<AosParticle> model(n);
    for (std::size_t i = 0; i < n; ++i) {
        const auto v = [&] { return static_cast<float>(rng.uniform(-50, 50)); };
        model[i] = {v(), v(), v(), v(), v(), v()};
        store.x()[i] = model[i].x;
        store.y()[i] = model[i].y;
        store.z()[i] = model[i].z;
        store.vx()[i] = model[i].vx;
        store.vy()[i] = model[i].vy;
        store.vz()[i] = model[i].vz;
    }
    aos_integrate(model.data(), n, 0.25F);
    store.integrate(0.25F);
    for (std::size_t i = 0; i < n; ++i) {
        INFO("i ", i);
        CHECK_NEAR(store.x()[i], model[i].x, 1e-3);
        CHECK_NEAR(store.y()[i], model[i].y, 1e-3);
        CHECK_NEAR(store.z()[i], model[i].z, 1e-3);
    }
}

PERF(5, "structure-of-arrays beats array-of-structures", 6000) {
    const std::size_t n = 1 << 16;
    Rng rng;
    std::vector<AosParticle> aos(n);
    ParticleStore soa(n);
    for (std::size_t i = 0; i < n; ++i) {
        const auto v = static_cast<float>(rng.uniform(1, 100));
        aos[i] = {1, 2, 3, v, v / 2, v / 3};
        soa.vx()[i] = v;
        soa.vy()[i] = v / 2;
        soa.vz()[i] = v / 3;
    }
    const double ratio = bench::speedup(
        5, [&] { for (int r = 0; r < 400; ++r) aos_integrate(aos.data(), n, 0.5F); },
        [&] { for (int r = 0; r < 400; ++r) soa.integrate(0.5F); });
    INFO("speedup ", ratio);
    CHECK_GE(ratio, 1.4);
}

TEST(6, "row_sums adds each row") {
    const std::vector<int> matrix = {1, 2, 3, 4, 5, 6};
    std::vector<int> out(2, 99);
    row_sums(matrix, 2, 3, out);
    CHECK_EQ(out, std::vector<int>({6, 15}));
}

TEST(6, "row_sums handles empty and rectangular shapes") {
    std::vector<int> out = {7};
    row_sums(std::span<const int>(), 0, 5, std::span<int>());
    row_sums(std::vector<int>{4}, 1, 1, out);
    CHECK_EQ(out[0], 4);
}

PERF(6, "row sums vectorize", 6000) {
    const std::size_t rows = 2048;
    const std::size_t cols = 3072;
    Rng rng;
    std::vector<int> matrix(rows * cols);
    for (int& value : matrix) {
        value = static_cast<int>(rng.uniform(-100, 100));
    }
    std::vector<int> out(rows);
    const double ratio = bench::speedup(
        5, [&] { for (int r = 0; r < 30; ++r) scalar_row_sums(matrix.data(), rows, cols, out.data()); },
        [&] { for (int r = 0; r < 30; ++r) row_sums(matrix, rows, cols, out); });
    INFO("speedup ", ratio);
    CHECK_GE(ratio, 3.0);
}

TEST(7, "best_match returns the row with the largest inner product") {
    const std::vector<float> query = {1, 2};
    const std::vector<float> matrix = {
        1, 0,   // 1
        0, 1,   // 2
        3, 3,   // 9
        -5, 1,  // -3
    };
    CHECK_EQ(best_match(query, matrix, 2), std::optional<std::size_t>(2));
}

TEST(7, "ties go to the smallest row and empty matrices have no match") {
    const std::vector<float> query = {1, 1};
    const std::vector<float> matrix = {2, 0, 1, 1, 0, 2};
    CHECK_EQ(best_match(query, matrix, 2), std::optional<std::size_t>(0));
    CHECK_EQ(best_match(query, std::span<const float>(), 2), std::optional<std::size_t>());
}

TEST(7, "negative scores are compared correctly") {
    const std::vector<float> query = {1};
    const std::vector<float> matrix = {-5, -1, -3};
    CHECK_EQ(best_match(query, matrix, 1), std::optional<std::size_t>(1));
}

TEST(7, "best_match agrees with brute force on random data") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        const auto dim = static_cast<std::size_t>(rng.uniform(1, 32));
        const auto rows = static_cast<std::size_t>(rng.uniform(1, 200));
        std::vector<float> query(dim);
        std::vector<float> matrix(rows * dim);
        for (float& value : query) {
            value = static_cast<float>(rng.uniform(-9, 9));
        }
        for (float& value : matrix) {
            value = static_cast<float>(rng.uniform(-9, 9));
        }
        std::size_t best_row = 0;
        float best_score = -std::numeric_limits<float>::infinity();
        for (std::size_t r = 0; r < rows; ++r) {
            float score = 0;
            for (std::size_t j = 0; j < dim; ++j) {
                score += query[j] * matrix[r * dim + j];
            }
            if (score > best_score) {
                best_score = score;
                best_row = r;
            }
        }
        INFO("round ", round);
        REQUIRE_EQ(best_match(query, matrix, dim), std::optional<std::size_t>(best_row));
    }
}

PERF(7, "brute-force search vectorizes the inner product", 6000) {
    const std::size_t rows = 20'000;
    const std::size_t dim = 128;
    Rng rng;
    std::vector<float> query(dim);
    std::vector<float> matrix(rows * dim);
    for (float& value : query) {
        value = static_cast<float>(rng.real());
    }
    for (float& value : matrix) {
        value = static_cast<float>(rng.real());
    }
    const double ratio = bench::speedup(
        5,
        [&] {
            for (int r = 0; r < 40; ++r) {
                bench::do_not_optimize(scalar_best_match(query.data(), matrix.data(), rows, dim));
            }
        },
        [&] {
            for (int r = 0; r < 40; ++r) {
                bench::do_not_optimize(best_match(query, matrix, dim));
            }
        });
    INFO("speedup ", ratio);
    CHECK_GE(ratio, 4.0);
}
