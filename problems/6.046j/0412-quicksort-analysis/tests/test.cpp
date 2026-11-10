#include <cmath>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;

namespace {

std::vector<int> iota_vector(std::size_t n) {
    std::vector<int> values(n);
    std::iota(values.begin(), values.end(), 0);
    return values;
}

std::vector<Counted<int>> to_counted(const std::vector<int>& values) {
    std::vector<Counted<int>> out;
    out.reserve(values.size());
    for (int value : values) {
        out.emplace_back(value);
    }
    return out;
}

double harmonic(std::size_t n) {
    double sum = 0.0;
    for (std::size_t i = 1; i <= n; ++i) {
        sum += 1.0 / static_cast<double>(i);
    }
    return sum;
}

double expected_total(std::size_t n) {
    const double real = static_cast<double>(n);
    return 2.0 * (real + 1.0) * harmonic(n) - 4.0 * real;
}

// A deliberately broken generator: every draw is zero, so the first element is always the pivot.
struct AlwaysZero {
    using result_type = std::uint64_t;
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
    result_type operator()() { return 0; }
};

std::vector<int> organ_pipe(std::size_t n) {
    std::vector<int> values(n);
    for (std::size_t i = 0; i < n; ++i) {
        values[i] = static_cast<int>(std::min(i, n - i));
    }
    return values;
}

}  // namespace

TEST(1, "a single element has one choice of pivot, itself") {
    CHECK_EQ(good_pivot_count(1, 0), 1);
}

TEST(1, "two elements: both pivots are good") {
    CHECK_EQ(good_pivot_count(2, 0), 2);
    CHECK_EQ(good_pivot_count(2, 1), 2);
}

TEST(1, "four elements at a middle rank") {
    CHECK_EQ(good_pivot_count(4, 1), 4);
}

TEST(1, "at least half of all pivots are good for every size and rank") {
    for (std::size_t m = 2; m <= 800; ++m) {
        for (std::size_t rank = 0; rank < m; ++rank) {
            INFO("m ", m, " rank ", rank);
            CHECK_GE(2 * good_pivot_count(m, rank), m);
        }
    }
}

TEST(1, "the count matches a direct enumeration") {
    for (std::size_t m = 2; m <= 300; ++m) {
        for (std::size_t rank = 0; rank < m; ++rank) {
            std::size_t expected = 0;
            for (std::size_t pivot = 0; pivot < m; ++pivot) {
                std::size_t next = 0;
                if (rank < pivot) {
                    next = pivot;
                } else if (rank > pivot) {
                    next = m - pivot - 1;
                }
                expected += (pivot == rank || 4 * next <= 3 * m) ? 1 : 0;
            }
            INFO("m ", m, " rank ", rank);
            CHECK_EQ(good_pivot_count(m, rank), expected);
        }
    }
}

TEST(2, "empty and single element inputs compare nothing") {
    Rng rng;
    std::vector<int> empty;
    const QuicksortTrace none = traced_quicksort<int>(empty, rng.engine());
    CHECK(none.pivot_comparisons.empty());
    CHECK_EQ(none.total_comparisons, 0);
    std::vector<int> one{1};
    const QuicksortTrace single = traced_quicksort<int>(one, rng.engine());
    CHECK_EQ(single.pivot_comparisons, std::vector<std::uint32_t>{0});
    CHECK_EQ(single.total_comparisons, 0);
}

TEST(2, "two elements make exactly one comparison") {
    Rng rng;
    std::vector<int> values{2, 1};
    const QuicksortTrace trace = traced_quicksort<int>(values, rng.engine());
    CHECK_EQ(values, (std::vector<int>{1, 2}));
    CHECK_EQ(trace.total_comparisons, 1);
    CHECK_EQ(trace.pivot_comparisons[0] + trace.pivot_comparisons[1], 1u);
}

TEST(2, "the result is sorted and every shape is handled") {
    Rng rng;
    for (const std::string shape : {"sorted", "reversed", "random"}) {
        INFO("shape ", shape);
        auto values = iota_vector(4096);
        if (shape == "reversed") {
            std::ranges::reverse(values);
        } else if (shape == "random") {
            rng.shuffle(values);
        }
        traced_quicksort<int>(values, rng.engine());
        CHECK(std::ranges::is_sorted(values));
    }
}

TEST(2, "counts stay indexed by the original position") {
    Rng rng;
    auto values = iota_vector(64);
    rng.shuffle(values);
    const std::vector<int> original = values;
    const QuicksortTrace trace = traced_quicksort<int>(values, rng.engine());
    REQUIRE_EQ(trace.pivot_comparisons.size(), original.size());
    std::uint64_t sum = 0;
    for (std::uint32_t count : trace.pivot_comparisons) {
        sum += count;
    }
    CHECK_EQ(sum, trace.total_comparisons);
}

TEST(2, "the trace matches the actual comparisons performed") {
    Rng rng;
    for (int trial = 0; trial < 50; ++trial) {
        INFO("trial ", trial);
        auto values = iota_vector(static_cast<std::size_t>(rng.uniform(2, 3000)));
        rng.shuffle(values);
        auto counted = to_counted(values);
        Counted<int>::comparisons() = 0;
        const QuicksortTrace trace = traced_quicksort<Counted<int>>(counted, rng.engine());
        CHECK_EQ(Counted<int>::comparisons(), trace.total_comparisons);
    }
}

TEST(2, "each element's mean comparison count is H_i + H_(n+1-i) - 2") {
    Rng rng;
    const std::size_t n = 200;
    const int trials = 4000;
    std::vector<double> sum(n, 0.0);
    for (int trial = 0; trial < trials; ++trial) {
        auto values = iota_vector(n);
        const QuicksortTrace trace = traced_quicksort<int>(values, rng.engine());
        for (std::size_t i = 0; i < n; ++i) {
            sum[i] += trace.pivot_comparisons[i];
        }
    }
    // The array starts sorted, so an element's original position equals its rank.
    for (std::size_t rank : {std::size_t{0}, n / 10, n / 2, n - 1}) {
        INFO("rank ", rank);
        const double expected = harmonic(rank + 1) + harmonic(n - rank) - 2.0;
        CHECK_NEAR(sum[rank] / trials, expected, 0.2);
    }
}

TEST(2, "the mean total matches the exact expectation") {
    Rng rng;
    const std::size_t n = 1000;
    const int trials = 400;
    double sum = 0.0;
    for (int trial = 0; trial < trials; ++trial) {
        auto values = iota_vector(n);
        rng.shuffle(values);
        sum += static_cast<double>(traced_quicksort<int>(values, rng.engine()).total_comparisons);
    }
    CHECK_NEAR(sum / trials, expected_total(n), 0.02 * expected_total(n));
}

TEST(3, "the per-element bound is 3(alpha + 1 + log_{4/3} 2) lg n") {
    const double c = std::log(2.0) / std::log(4.0 / 3.0);
    CHECK_NEAR(element_bound(1024, 1), 3.0 * (2.0 + c) * 10.0, 1e-9);
    CHECK_NEAR(element_bound(1 << 20, 3), 3.0 * (4.0 + c) * 20.0, 1e-9);
    CHECK_NEAR(element_bound(2, 1), 3.0 * (2.0 + c), 1e-9);
}

TEST(3, "the total bound is n times the per-element bound") {
    CHECK_NEAR(total_bound(4096, 2), 4096.0 * element_bound(4096, 2), 1e-6);
}

TEST(3, "the bound for alpha one is below 14 lg n") {
    CHECK_LE(element_bound(1 << 16, 1), 14.0 * 16.0);
}

TEST(3, "larger alpha means a larger bound") {
    CHECK_GT(element_bound(4096, 3), element_bound(4096, 2));
}

TEST(3, "random trials never exceed the bound") {
    Rng rng;
    for (const std::size_t n : {std::size_t{64}, std::size_t{1024}, std::size_t{8192}}) {
        INFO("n ", n);
        const BoundReport report = measure_bound(n, 200, 1, rng.engine());
        CHECK_EQ(report.trials, 200);
        CHECK_EQ(report.element_violations, 0);
        CHECK_EQ(report.total_violations, 0);
    }
}

TEST(3, "the measured mean agrees with the exact expectation") {
    Rng rng;
    const std::size_t n = 2000;
    const BoundReport report = measure_bound(n, 300, 1, rng.engine());
    CHECK_NEAR(report.mean_total, expected_total(n), 0.03 * expected_total(n));
}

TEST(3, "a tighter alpha cannot produce more violations") {
    Rng rng;
    const BoundReport loose = measure_bound(1024, 100, 3, rng.engine());
    CHECK_EQ(loose.element_violations, 0);
    CHECK_EQ(loose.total_violations, 0);
}

TEST(4, "short inputs are handled") {
    Rng rng;
    std::vector<int> empty;
    guarded_sort<int>(empty, rng.engine());
    std::vector<int> one{1};
    guarded_sort<int>(one, rng.engine());
    CHECK_EQ(one, std::vector<int>{1});
    std::vector<int> two{2, 1};
    guarded_sort<int>(two, rng.engine());
    CHECK_EQ(two, (std::vector<int>{1, 2}));
}

TEST(4, "duplicates and every shape sort correctly") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        INFO("trial ", trial);
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 500)), 0, rng.coin() ? 3 : 100000);
        auto expected = values;
        std::ranges::sort(expected);
        guarded_sort<int>(values, rng.engine());
        REQUIRE_EQ(values, expected);
    }
}

TEST(4, "strings sort without losing data") {
    Rng rng;
    std::vector<std::string> words;
    for (int i = 0; i < 2000; ++i) {
        words.push_back(rng.text(static_cast<std::size_t>(rng.uniform(0, 30)), "ab"));
    }
    auto expected = words;
    std::ranges::sort(expected);
    guarded_sort<std::string>(words, rng.engine());
    CHECK_EQ(words, expected);
}

TEST(4, "comparisons stay O(n log n) even with a broken generator") {
    const std::size_t n = 50'000;
    const double bound = 6.0 * static_cast<double>(n) * std::log2(static_cast<double>(n));
    std::vector<int> sorted = iota_vector(n);
    std::vector<int> reversed(sorted.rbegin(), sorted.rend());
    for (const std::vector<int>* input : {&sorted, &reversed}) {
        auto counted = to_counted(*input);
        AlwaysZero broken;
        Counted<int>::comparisons() = 0;
        guarded_sort<Counted<int>>(counted, broken);
        CHECK_LE(static_cast<double>(Counted<int>::comparisons()), bound);
        for (std::size_t i = 1; i < counted.size(); ++i) {
            REQUIRE(counted[i - 1].value() <= counted[i].value());
        }
    }
    auto pipe = to_counted(organ_pipe(n));
    AlwaysZero broken;
    Counted<int>::comparisons() = 0;
    guarded_sort<Counted<int>>(pipe, broken);
    CHECK_LE(static_cast<double>(Counted<int>::comparisons()), bound);
}

TEST(4, "a healthy generator rarely needs the fallback") {
    Rng rng;
    const std::size_t n = 1 << 14;
    std::uint64_t total = 0;
    for (int trial = 0; trial < 10; ++trial) {
        auto values = iota_vector(n);
        rng.shuffle(values);
        auto counted = to_counted(values);
        Counted<int>::comparisons() = 0;
        guarded_sort<Counted<int>>(counted, rng.engine());
        total += Counted<int>::comparisons();
    }
    CHECK_LE(static_cast<double>(total) / 10.0, 3.0 * static_cast<double>(n) * 14.0);
}

PERF(4, "guarded sort of three million integers", 3000) {
    Rng rng;
    auto values = rng.ints(3'000'000, 0, 1'000'000'000);
    guarded_sort<int>(values, rng.engine());
    CHECK(std::ranges::is_sorted(values));
}
