#include <cmath>
#include <vector>

#include "08_cache_sim.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::growth_exponent;
using practice::cachesim::Cache;
using practice::cachesim::Memory;
using practice::cachesim::PlainArray;
using practice::cachesim::TracedArray;

namespace {

// Exact values for any f: fill a full n×n table directly.
template <typename F>
double full_tableau(std::size_t n, double init, F f) {
    std::vector<std::vector<double>> table(n, std::vector<double>(n, init));
    for (std::size_t i = 1; i < n; ++i) {
        for (std::size_t j = 1; j < n; ++j) {
            table[i][j] = f(table[i - 1][j - 1], table[i][j - 1], table[i - 1][j]);
        }
    }
    return table[n - 1][n - 1];
}

// Array of exactly the size asked for that rejects out-of-range access.
class CheckedArray {
public:
    explicit CheckedArray(std::size_t size) : data_(size, -123456.0) {}
    std::size_t size() const { return data_.size(); }
    double get(std::size_t index) const {
        REQUIRE(index < data_.size());
        return data_[index];
    }
    void set(std::size_t index, double value) {
        REQUIRE(index < data_.size());
        data_[index] = value;
    }

private:
    std::vector<double> data_;
};

// Combines the three neighbours with distinct weights, keeping values exact integers.
double mix(double up_left, double left, double up) {
    return std::fmod(up_left * 31.0 + left * 37.0 + up * 41.0 + 7.0, 1'000'000'007.0);
}

constexpr std::size_t kCapacity = 4096;
constexpr std::size_t kLine = 64;
const CacheGeometry kGeometry{512.0, 8.0};

std::uint64_t iterative_misses(std::size_t n) {
    Cache cache(kCapacity, kLine, kCapacity / kLine);
    Memory memory(cache);
    TracedArray<double> a(memory, 2 * n - 1);
    cache.reset_counters();
    fill_tableau(a, n, 1.0, mix);
    return cache.misses();
}

}  // namespace

TEST(1, "diagonal_index maps the corners of the tableau") {
    CHECK_EQ(diagonal_index(0, 0, 5), 4);
    CHECK_EQ(diagonal_index(4, 0, 5), 8);
    CHECK_EQ(diagonal_index(0, 4, 5), 0);
    CHECK_EQ(diagonal_index(4, 4, 5), 4);
}

TEST(1, "cells on one diagonal share a slot, neighbours do not") {
    CHECK_EQ(diagonal_index(2, 1, 6), diagonal_index(3, 2, 6));
    CHECK_NE(diagonal_index(2, 1, 6), diagonal_index(2, 2, 6));
    CHECK_NE(diagonal_index(2, 1, 6), diagonal_index(3, 1, 6));
}

TEST(1, "n equal to one returns the initial value") {
    CheckedArray a(1);
    CHECK_NEAR(fill_tableau(a, 1, 5.0, mix), 5.0, 1e-12);
}

TEST(1, "n equal to two computes the single interior cell") {
    CheckedArray a(3);
    CHECK_NEAR(fill_tableau(a, 2, 2.0, [](double p, double q, double r) { return p + 10 * q + 100 * r; }),
               2.0 + 20.0 + 200.0, 1e-12);
}

TEST(1, "the result matches a full table for many sizes") {
    Rng rng;
    for (std::size_t n = 1; n <= 40; ++n) {
        INFO("n=", n);
        const double init = static_cast<double>(rng.uniform(1, 50));
        CheckedArray a(2 * n - 1);
        CHECK_NEAR(fill_tableau(a, n, init, mix), full_tableau(n, init, mix), 1e-9);
    }
}

TEST(1, "f receives exactly the three named neighbours") {
    const std::size_t n = 6;
    std::vector<std::vector<double>> table(n, std::vector<double>(n, 1.0));
    std::vector<std::tuple<double, double, double>> expected_calls;
    for (std::size_t i = 1; i < n; ++i) {
        for (std::size_t j = 1; j < n; ++j) {
            expected_calls.emplace_back(table[i - 1][j - 1], table[i][j - 1], table[i - 1][j]);
            table[i][j] = mix(table[i - 1][j - 1], table[i][j - 1], table[i - 1][j]);
        }
    }
    std::size_t call = 0;
    PlainArray<double> a(2 * n - 1);
    fill_tableau(a, n, 1.0, [&](double p, double q, double r) {
        INFO("call ", call);
        REQUIRE(call < expected_calls.size());
        const auto [ep, eq, er] = expected_calls[call];
        CHECK_NEAR(p, ep, 1e-9);
        CHECK_NEAR(q, eq, 1e-9);
        CHECK_NEAR(r, er, 1e-9);
        ++call;
        return mix(p, q, r);
    });
    CHECK_EQ(call, expected_calls.size());
}

TEST(2, "a tableau that fits in cache only pays for cold misses") {
    for (const std::size_t n : {64, 128, 200, 300}) {
        INFO("n=", n);
        const auto actual = static_cast<double>(iterative_misses(n));
        const double predicted = predict_iterative_misses(static_cast<double>(n), kGeometry);
        CHECK_GT(predicted * 2, actual);
        CHECK_GT(actual * 2, predicted);
    }
}

TEST(2, "a tableau larger than the cache pays per row") {
    for (const std::size_t n : {700, 1000, 1400}) {
        INFO("n=", n);
        const auto actual = static_cast<double>(iterative_misses(n));
        const double predicted = predict_iterative_misses(static_cast<double>(n), kGeometry);
        CHECK_GT(predicted * 2, actual);
        CHECK_GT(actual * 2, predicted);
    }
}

TEST(2, "misses grow linearly below the cache and quadratically above it") {
    const double small = growth_exponent(static_cast<double>(iterative_misses(64)),
                                         static_cast<double>(iterative_misses(200)), 64, 200);
    INFO("small regime exponent ", small);
    CHECK_GT(small, 0.8);
    CHECK_LT(small, 1.2);

    const double large = growth_exponent(static_cast<double>(iterative_misses(700)),
                                         static_cast<double>(iterative_misses(1400)), 700, 1400);
    INFO("large regime exponent ", large);
    CHECK_GT(large, 1.8);
    CHECK_LT(large, 2.2);
}
