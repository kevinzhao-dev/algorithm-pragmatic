#include <algorithm>
#include <bit>
#include <functional>
#include <numeric>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

struct Mat2 {
    long long a = 1;
    long long b = 0;
    long long c = 0;
    long long d = 1;
    friend bool operator==(const Mat2&, const Mat2&) = default;
};

constexpr long long kMod = 1'000'000'007;

Mat2 multiply(const Mat2& l, const Mat2& r) {
    return {(l.a * r.a + l.b * r.c) % kMod, (l.a * r.b + l.b * r.d) % kMod,
            (l.c * r.a + l.d * r.c) % kMod, (l.c * r.b + l.d * r.d) % kMod};
}

std::ostream& operator<<(std::ostream& os, const Mat2& m) {
    return os << '[' << m.a << ' ' << m.b << ' ' << m.c << ' ' << m.d << ']';
}

const auto concat = [](const std::string& a, const std::string& b) { return a + b; };
const auto add = [](int a, int b) { return a + b; };

std::vector<std::string> random_words(Rng& rng, std::size_t n) {
    std::vector<std::string> words(n);
    for (auto& word : words) {
        word = rng.text(static_cast<std::size_t>(rng.uniform(0, 3)), "abc");
    }
    return words;
}

std::vector<Mat2> random_matrices(Rng& rng, std::size_t n) {
    std::vector<Mat2> values(n);
    for (Mat2& m : values) {
        m = {rng.uniform(0, 9), rng.uniform(0, 9), rng.uniform(0, 9), rng.uniform(0, 9)};
    }
    return values;
}

template <typename T, typename Op>
std::vector<T> serial_scan(const std::vector<T>& x, Op op) {
    std::vector<T> y(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        y[i] = i == 0 ? x[0] : op(y[i - 1], x[i]);
    }
    return y;
}

std::size_t ceil_lg(std::size_t n) {
    return n <= 1 ? 0 : static_cast<std::size_t>(std::bit_width(n - 1));
}

long long op_count(const exec::Meter& meter) { return meter.work() - meter.spawns(); }

template <typename Scan>
void check_scan_values(Scan scan) {
    Rng rng;
    {
        exec::Meter meter;
        const std::vector<std::string> empty;
        CHECK(scan(meter, std::span<const std::string>(empty), concat).empty());
    }
    for (const std::size_t n : {1, 2, 3, 4, 5, 8, 9, 17, 64, 200}) {
        const std::vector<std::string> words = random_words(rng, n);
        exec::Meter meter;
        const auto y = scan(meter, std::span<const std::string>(words), concat);
        INFO("n ", n);
        REQUIRE_EQ(y, serial_scan(words, concat));
    }
    for (const std::size_t n : {1, 2, 16, 33, 100}) {
        const std::vector<Mat2> values = random_matrices(rng, n);
        exec::Meter meter;
        const auto y = scan(meter, std::span<const Mat2>(values), multiply);
        INFO("n ", n);
        CHECK(y == serial_scan(values, multiply));
    }
}

template <typename Scan>
void check_scan_on_pool(Scan scan) {
    Rng rng;
    exec::ForkJoin pool;
    for (const unsigned workers : {1U, 4U, 8U}) {
        practice::set_workers(workers);
        for (const std::size_t n : {1, 2, 100, 3000}) {
            const std::vector<int> values = rng.ints(n, -50, 50);
            const auto y = scan(pool, std::span<const int>(values), add);
            INFO("workers ", workers, " n ", n);
            REQUIRE_EQ(y, serial_scan(values, add));
        }
        const std::vector<std::string> words = random_words(rng, 2000);
        const auto y = scan(pool, std::span<const std::string>(words), concat);
        REQUIRE_EQ(y, serial_scan(words, concat));
    }
}

}  // namespace

TEST(1, "an empty input is rejected") {
    exec::Meter meter;
    const std::vector<int> empty;
    CHECK_THROWS_AS(p_reduce(meter, std::span<const int>(empty), add), std::invalid_argument);
}

TEST(1, "a single element is returned without calling op") {
    exec::Meter meter;
    const std::vector<int> one = {7};
    CHECK_EQ(p_reduce(meter, std::span<const int>(one), add), 7);
    CHECK_EQ(op_count(meter), 0);
}

TEST(1, "non-commutative operators keep left-to-right order") {
    exec::Meter meter;
    const std::vector<std::string> words = {"re", "du", "ce", "!"};
    CHECK_EQ(p_reduce(meter, std::span<const std::string>(words), concat), "reduce!");
    Rng rng;
    for (const std::size_t n : {1, 2, 3, 10, 101}) {
        const std::vector<Mat2> values = random_matrices(rng, n);
        Mat2 expected = values[0];
        for (std::size_t i = 1; i < n; ++i) {
            expected = multiply(expected, values[i]);
        }
        exec::Meter m;
        INFO("n ", n);
        CHECK(p_reduce(m, std::span<const Mat2>(values), multiply) == expected);
    }
}

TEST(1, "work is n-1 ops and span is logarithmic") {
    for (const std::size_t n : {1, 2, 3, 100, 1024, 4097, 1 << 16}) {
        const std::vector<int> values(n, 1);
        exec::Meter meter;
        CHECK_EQ(p_reduce(meter, std::span<const int>(values), add), static_cast<int>(n));
        INFO("n ", n, " span ", meter.span());
        CHECK_EQ(op_count(meter), static_cast<long long>(n - 1));
        CHECK_LE(meter.span(), static_cast<long long>(3 * ceil_lg(n) + 4));
    }
}

TEST(1, "no race on the real pool") {
    Rng rng;
    exec::ForkJoin pool;
    for (const unsigned workers : {1U, 4U, 8U}) {
        practice::set_workers(workers);
        const std::vector<int> values = rng.ints(200'000, -100, 100);
        const int expected = std::accumulate(values.begin(), values.end(), 0);
        CHECK_EQ(p_reduce(pool, std::span<const int>(values), add), expected);
        const std::vector<std::string> words = random_words(rng, 3000);
        CHECK_EQ(p_reduce(pool, std::span<const std::string>(words), concat),
                 std::accumulate(words.begin(), words.end(), std::string()));
    }
}

TEST(2, "scan values match a serial scan") {
    check_scan_values([](auto& e, auto x, auto op) { return p_scan_1(e, x, op); });
}

TEST(2, "work is quadratic and span logarithmic") {
    for (const std::size_t n : {1, 2, 8, 100, 512}) {
        const std::vector<int> values(n, 1);
        exec::Meter meter;
        (void)p_scan_1(meter, std::span<const int>(values), add);
        INFO("n ", n, " span ", meter.span());
        CHECK_EQ(op_count(meter), static_cast<long long>(n * (n - 1) / 2));
        CHECK_LE(meter.span(), static_cast<long long>(6 * ceil_lg(n) + 8));
    }
}

TEST(2, "no race on the real pool") {
    check_scan_on_pool([](auto& e, auto x, auto op) { return p_scan_1(e, x, op); });
}

TEST(3, "scan values match a serial scan") {
    check_scan_values([](auto& e, auto x, auto op) { return p_scan_2(e, x, op); });
}

TEST(3, "ops are n lg n / 2 for powers of two") {
    for (const std::size_t n : {2, 4, 8, 64, 1024, 1 << 14}) {
        const std::vector<int> values(n, 1);
        exec::Meter meter;
        (void)p_scan_2(meter, std::span<const int>(values), add);
        INFO("n ", n);
        CHECK_EQ(op_count(meter), static_cast<long long>(n / 2 * ceil_lg(n)));
    }
}

TEST(3, "span is polylogarithmic, not linear") {
    for (const std::size_t n : {8, 256, 4096, 1 << 15}) {
        const std::vector<int> values(n, 1);
        exec::Meter meter;
        (void)p_scan_2(meter, std::span<const int>(values), add);
        const auto lg = static_cast<long long>(ceil_lg(n));
        INFO("n ", n, " span ", meter.span());
        CHECK_LE(meter.span(), 3 * lg * lg + 16);
    }
}

TEST(3, "no race on the real pool") {
    check_scan_on_pool([](auto& e, auto x, auto op) { return p_scan_2(e, x, op); });
}

TEST(4, "scan values match a serial scan") {
    check_scan_values([](auto& e, auto x, auto op) { return p_scan_3(e, x, op); });
}

TEST(4, "work is linear") {
    for (const std::size_t n : {1, 2, 3, 100, 1000, 1 << 16}) {
        const std::vector<int> values(n, 1);
        exec::Meter meter;
        (void)p_scan_3(meter, std::span<const int>(values), add);
        INFO("n ", n, " ops ", op_count(meter));
        CHECK_LE(op_count(meter), static_cast<long long>(3 * n));
    }
}

TEST(4, "span is logarithmic") {
    for (const std::size_t n : {2, 16, 1024, 1 << 16}) {
        const std::vector<int> values(n, 1);
        exec::Meter meter;
        (void)p_scan_3(meter, std::span<const int>(values), add);
        INFO("n ", n, " span ", meter.span());
        CHECK_LE(meter.span(), static_cast<long long>(6 * ceil_lg(n) + 12));
    }
}

TEST(4, "no race on the real pool") {
    check_scan_on_pool([](auto& e, auto x, auto op) { return p_scan_3(e, x, op); });
}

TEST(5, "filter keeps matching elements in order") {
    exec::Meter meter;
    const std::vector<int> values = {5, 2, 8, 1, 9, 4};
    const auto kept = parallel_filter(meter, std::span<const int>(values),
                                      [](int v) { return v % 2 == 0; });
    CHECK_EQ(kept, std::vector<int>({2, 8, 4}));
}

TEST(5, "filter handles none, all and empty") {
    exec::Meter meter;
    const std::vector<int> values = {1, 3, 5};
    CHECK(parallel_filter(meter, std::span<const int>(values), [](int) { return false; }).empty());
    CHECK_EQ(parallel_filter(meter, std::span<const int>(values), [](int) { return true; }), values);
    CHECK(parallel_filter(meter, std::span<const int>(), [](int) { return true; }).empty());
}

TEST(5, "filter matches std::copy_if on random data") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(0, 500));
        const std::vector<int> values = rng.ints(n, 0, 20);
        const auto pred = [](int v) { return v % 3 == 1; };
        std::vector<int> expected;
        std::copy_if(values.begin(), values.end(), std::back_inserter(expected), pred);
        exec::Meter meter;
        INFO("round ", round);
        REQUIRE_EQ(parallel_filter(meter, std::span<const int>(values), pred), expected);
    }
}

TEST(5, "filter work is linear and span logarithmic") {
    for (const std::size_t n : {16, 1024, 1 << 16}) {
        const std::vector<int> values(n, 1);
        exec::Meter meter;
        (void)parallel_filter(meter, std::span<const int>(values), [](int) { return true; });
        INFO("n ", n, " ops ", op_count(meter), " span ", meter.span());
        CHECK_LE(op_count(meter), static_cast<long long>(6 * n));
        CHECK_LE(meter.span(), static_cast<long long>(10 * ceil_lg(n) + 24));
    }
}

TEST(5, "filter has no race on the real pool") {
    Rng rng;
    exec::ForkJoin pool;
    practice::set_workers(8);
    const std::vector<int> values = rng.ints(50'000, 0, 1000);
    const auto pred = [](int v) { return v < 300; };
    std::vector<int> expected;
    std::copy_if(values.begin(), values.end(), std::back_inserter(expected), pred);
    CHECK_EQ(parallel_filter(pool, std::span<const int>(values), pred), expected);
}
