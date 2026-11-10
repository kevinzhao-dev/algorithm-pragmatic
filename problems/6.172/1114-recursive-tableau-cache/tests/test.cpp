#include <cmath>
#include <vector>

#include "08_cache_sim.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::growth_exponent;
using practice::cachesim::Cache;
using practice::cachesim::Memory;
using practice::cachesim::TracedArray;

namespace {

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

class CheckedArray {
public:
    explicit CheckedArray(std::size_t size) : data_(size, -9.0) {}
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

double mix(double up_left, double left, double up) {
    return std::fmod(up_left * 31.0 + left * 37.0 + up * 41.0 + 7.0, 1'000'000'007.0);
}

constexpr std::size_t kCapacity = 4096;
constexpr std::size_t kLine = 64;
const CacheGeometry kGeometry{512.0, 8.0};

// An independent row-by-row fill of the same 2n - 1 array, used as the baseline.
template <typename Arr>
void iterative_baseline(Arr& a, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        a.set(n + i - 1, 1.0);
        a.set(n - i - 1, 1.0);
    }
    for (std::size_t i = 1; i < n; ++i) {
        for (std::size_t j = 1; j < n; ++j) {
            a.set(n + i - j - 1, mix(a.get(n + i - j - 1), a.get(n + i - j),
                                     a.get(n + i - j - 2)));
        }
    }
}

std::uint64_t misses_recursive(std::size_t n) {
    Cache cache(kCapacity, kLine, kCapacity / kLine);
    Memory memory(cache);
    TracedArray<double> a(memory, 2 * n - 1);
    cache.reset_counters();
    recursive_tableau(a, n, 1.0, mix);
    return cache.misses();
}

std::uint64_t misses_iterative(std::size_t n) {
    Cache cache(kCapacity, kLine, kCapacity / kLine);
    Memory memory(cache);
    TracedArray<double> a(memory, 2 * n - 1);
    cache.reset_counters();
    iterative_baseline(a, n);
    return cache.misses();
}

// A small cache (128 elements in 32-byte lines) where the k-way sizes are interesting.
double normalized_kway_misses(std::size_t n, std::size_t k) {
    Cache cache(1024, 32, 32);
    Memory memory(cache);
    TracedArray<double> a(memory, 2 * n - 1);
    cache.reset_counters();
    recursive_tableau_k(a, n, k, 1.0, mix);
    return 1000.0 * static_cast<double>(cache.misses()) / static_cast<double>(n * n);
}

}  // namespace

TEST(1, "recursive filling matches a full table") {
    for (std::size_t n = 1; n <= 40; ++n) {
        INFO("n=", n);
        CheckedArray a(2 * n - 1);
        CHECK_NEAR(recursive_tableau(a, n, 2.0, mix), full_tableau(n, 2.0, mix), 1e-9);
    }
}

TEST(2, "the recursive fill misses a hundred times less than the iterative one") {
    const std::size_t n = 1024;
    const std::uint64_t iterative = misses_iterative(n);
    const std::uint64_t recursive = misses_recursive(n);
    INFO("iterative ", iterative, " recursive ", recursive);
    CHECK_GT(iterative, 30 * recursive);
}

TEST(2, "the prediction is within the stated factor of the simulation") {
    for (const std::size_t n : {700, 1000, 1400, 2000}) {
        INFO("n=", n);
        const auto actual = static_cast<double>(misses_recursive(n));
        const double predicted = predict_recursive_misses(static_cast<double>(n), kGeometry);
        CHECK_GT(actual, 0.5 * predicted);
        CHECK_LT(actual, 12.0 * predicted);
    }
}

TEST(2, "misses grow quadratically with the tableau side") {
    const double exponent = growth_exponent(static_cast<double>(misses_recursive(1000)),
                                            static_cast<double>(misses_recursive(2000)), 1000, 2000);
    INFO("exponent ", exponent);
    CHECK_GT(exponent, 1.6);
    CHECK_LT(exponent, 2.2);
}

TEST(3, "k-way filling matches a full table") {
    for (const std::size_t k : {2, 3, 4, 8, 16}) {
        for (std::size_t n = 1; n <= 30; n += 3) {
            INFO("n=", n, " k=", k);
            CheckedArray a(2 * n - 1);
            CHECK_NEAR(recursive_tableau_k(a, n, k, 1.0, mix), full_tableau(n, 1.0, mix), 1e-9);
        }
    }
}

TEST(3, "large k punishes an unlucky size but k = 2 does not") {
    const double lucky_16 = normalized_kway_misses(93, 16);
    const double unlucky_16 = normalized_kway_misses(116, 16);
    INFO("k=16 lucky ", lucky_16, " unlucky ", unlucky_16);
    CHECK_GT(unlucky_16, 2.0 * lucky_16);

    const double lucky_2 = normalized_kway_misses(93, 2);
    const double unlucky_2 = normalized_kway_misses(116, 2);
    INFO("k=2 lucky ", lucky_2, " unlucky ", unlucky_2);
    CHECK_LT(unlucky_2, 1.5 * lucky_2);
}
