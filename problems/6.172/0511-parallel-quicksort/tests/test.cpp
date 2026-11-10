#include <algorithm>
#include <numeric>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

template <typename T, typename Compare = std::less<>>
void check_partitioned(const std::vector<T>& original, const std::vector<T>& data, std::size_t p,
                       const T& pivot, Compare cmp = {}) {
    REQUIRE(p < data.size());
    CHECK_EQ(data[p], pivot);
    for (std::size_t i = 0; i < p; ++i) {
        CHECK(!cmp(pivot, data[i]));
    }
    for (std::size_t i = p + 1; i < data.size(); ++i) {
        CHECK(!cmp(data[i], pivot));
    }
    std::vector<T> a = original;
    std::vector<T> b = data;
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    CHECK_EQ(a, b);
}

std::vector<int> random_ints(Rng& rng, std::size_t n, int low, int high) {
    return rng.ints(n, low, high);
}

template <typename Exec, typename Compare = std::less<>>
void check_sorts(Exec& exec, std::size_t n, Rng& rng, Compare cmp = {}) {
    std::vector<int> values = random_ints(rng, n, -1000, 1000);
    std::vector<int> expected = values;
    std::sort(expected.begin(), expected.end(), cmp);
    quicksort(exec, std::span<int>(values), cmp);
    INFO("n ", n);
    CHECK_EQ(values, expected);
}

}  // namespace

TEST(1, "pivot lands at its final position on a small example") {
    std::vector<int> data = {5, 2, 9, 1, 7};
    const std::vector<int> original = data;
    const std::size_t p = partition_around(std::span<int>(data), 0);
    check_partitioned(original, data, p, 5);
    CHECK_EQ(p, 2);
}

TEST(1, "every pivot index works") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 40));
        std::vector<int> data = random_ints(rng, n, 0, 8);
        const std::vector<int> original = data;
        const auto pivot_index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
        const int pivot = data[pivot_index];
        const std::size_t p = partition_around(std::span<int>(data), pivot_index);
        INFO("round ", round, " n ", n, " pivot_index ", pivot_index);
        check_partitioned(original, data, p, pivot);
    }
}

TEST(1, "single element and custom comparator") {
    std::vector<int> one = {3};
    CHECK_EQ(partition_around(std::span<int>(one), 0), 0);
    std::vector<int> data = {1, 2, 3, 4, 5};
    const std::vector<int> original = data;
    const std::size_t p = partition_around(std::span<int>(data), 4, std::greater<>{});
    check_partitioned(original, data, p, 5, std::greater<>{});
}

TEST(2, "sorts correctly under the meter") {
    Rng rng;
    exec::Meter meter;
    for (const std::size_t n : {0, 1, 2, 3, 10, 100, 1000}) {
        check_sorts(meter, n, rng);
    }
}

TEST(2, "sorts with a custom comparator") {
    Rng rng;
    exec::Meter meter;
    check_sorts(meter, 500, rng, std::greater<>{});
}

TEST(2, "sorted reversed and constant inputs do not degrade the result") {
    exec::Meter meter;
    std::vector<int> sorted(20'000);
    std::iota(sorted.begin(), sorted.end(), 0);
    std::vector<int> input = sorted;
    quicksort(meter, std::span<int>(input));
    CHECK_EQ(input, sorted);
    std::reverse(input.begin(), input.end());
    quicksort(meter, std::span<int>(input));
    CHECK_EQ(input, sorted);
    std::vector<int> constant(20'000, 7);
    quicksort(meter, std::span<int>(constant));
    CHECK_EQ(constant, std::vector<int>(20'000, 7));
}

TEST(2, "the median-of-three rule keeps sorted input fast") {
    exec::Meter meter;
    std::vector<int> sorted(50'000);
    std::iota(sorted.begin(), sorted.end(), 0);
    quicksort(meter, std::span<int>(sorted));
    // With median of three, already sorted input splits evenly: nowhere near quadratic.
    CHECK_LT(meter.work(), 50'000LL * 20);
}

TEST(2, "work is near n log n and parallelism grows with n") {
    Rng rng;
    const std::vector<int> big = random_ints(rng, 1 << 16, 0, 1 << 20);
    const std::vector<int> small = random_ints(rng, 1 << 10, 0, 1 << 20);
    exec::Meter meter_big;
    std::vector<int> a = big;
    quicksort(meter_big, std::span<int>(a));
    exec::Meter meter_small;
    std::vector<int> b = small;
    quicksort(meter_small, std::span<int>(b));
    INFO("big work ", meter_big.work(), " span ", meter_big.span(), " small span ", meter_small.span());
    CHECK_LT(meter_big.work(), static_cast<long long>(40 * big.size() * 16));
    // Serial partitioning at the top makes span linear in n.
    CHECK_GE(meter_big.span(), static_cast<long long>(big.size()));
    CHECK_GT(meter_big.parallelism(), meter_small.parallelism());
    CHECK_GT(meter_big.parallelism(), 3.0);
}

TEST(2, "no race on the real pool") {
    Rng rng;
    exec::ForkJoin pool;
    for (const unsigned workers : {1U, 4U, 8U}) {
        practice::set_workers(workers);
        for (const std::size_t n : {2, 100, 5000, 50'000}) {
            std::vector<int> values = random_ints(rng, n, 0, 1000);
            std::vector<int> expected = values;
            std::sort(expected.begin(), expected.end());
            quicksort(pool, std::span<int>(values));
            INFO("workers ", workers, " n ", n);
            CHECK_EQ(values, expected);
        }
    }
}

TEST(3, "cutoff variants still sort everything") {
    Rng rng;
    exec::Meter meter;
    for (const std::size_t cutoff : {1, 2, 8, 64, 1000000}) {
        for (const std::size_t n : {0, 1, 7, 64, 65, 999, 5000}) {
            std::vector<int> values = random_ints(rng, n, -50, 50);
            std::vector<int> expected = values;
            std::sort(expected.begin(), expected.end());
            quicksort(meter, std::span<int>(values), std::less<>{}, cutoff);
            INFO("cutoff ", cutoff, " n ", n);
            CHECK_EQ(values, expected);
        }
    }
}

TEST(3, "a larger cutoff means fewer spawns") {
    Rng rng;
    const std::vector<int> data = random_ints(rng, 1 << 14, 0, 1 << 20);
    const auto spawns_for = [&](std::size_t cutoff) {
        exec::Meter meter;
        std::vector<int> values = data;
        quicksort(meter, std::span<int>(values), std::less<>{}, cutoff);
        return meter.spawns();
    };
    const auto fine = spawns_for(1);
    const auto coarse = spawns_for(64);
    INFO("fine spawns ", fine, " coarse spawns ", coarse);
    CHECK_GT(fine, coarse * 4);
    CHECK_LT(coarse, static_cast<long long>(4 * data.size() / 64 + 32));
}

TEST(3, "no race on the real pool with coarsening") {
    Rng rng;
    exec::ForkJoin pool;
    practice::set_workers(8);
    std::vector<int> values = random_ints(rng, 100'000, 0, 5000);
    std::vector<int> expected = values;
    std::sort(expected.begin(), expected.end());
    quicksort(pool, std::span<int>(values), std::less<>{}, 256);
    CHECK_EQ(values, expected);
}
