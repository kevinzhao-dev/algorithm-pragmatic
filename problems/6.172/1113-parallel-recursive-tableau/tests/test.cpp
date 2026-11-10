#include <cmath>
#include <vector>

#include "08_cache_sim.hpp"
#include "forkjoin.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::cachesim::PlainArray;

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

// Rejects out-of-range access: the array must not be bigger than 2n - 1.
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

// The span of the k²-way recursion computed from its structure, without the closed form.
std::uint64_t span_by_recursion(std::size_t n, std::size_t k) {
    if (n == 1) {
        return 1;
    }
    std::uint64_t total = 0;
    for (std::size_t diagonal = 0; diagonal < 2 * k - 1; ++diagonal) {
        std::uint64_t slowest = 0;
        for (std::size_t p = 0; p < k; ++p) {
            if (diagonal >= p && diagonal - p < k) {
                slowest = std::max(slowest, span_by_recursion(n / k, k));
            }
        }
        total += slowest;
    }
    return total;
}

}  // namespace

TEST(1, "recursive filling matches a full table") {
    for (std::size_t n = 1; n <= 32; ++n) {
        INFO("n=", n);
        CheckedArray a(2 * n - 1);
        CHECK_NEAR(recursive_tableau(a, n, 3.0, mix), full_tableau(n, 3.0, mix), 1e-9);
    }
}

TEST(1, "the smallest cases work without recursion artifacts") {
    CheckedArray one(1);
    CHECK_NEAR(recursive_tableau(one, 1, 4.0, mix), 4.0, 1e-12);
    CheckedArray two(3);
    CHECK_NEAR(recursive_tableau(two, 2, 2.0, mix), full_tableau(2, 2.0, mix), 1e-9);
}

TEST(1, "non-square sub-ranges are handled") {
    for (std::size_t n : {3, 5, 7, 11, 13, 17}) {
        INFO("n=", n);
        CheckedArray a(2 * n - 1);
        CHECK_NEAR(recursive_tableau(a, n, 1.0, mix), full_tableau(n, 1.0, mix), 1e-9);
    }
}

TEST(2, "the parallel result equals the serial result") {
    practice::set_workers(4);
    for (std::size_t n : {std::size_t{1}, std::size_t{2}, std::size_t{33}, std::size_t{200}, std::size_t{400}}) {
        INFO("n=", n);
        PlainArray<double> parallel(2 * n - 1);
        CHECK_NEAR(parallel_tableau(parallel, n, 1.0, mix), full_tableau(n, 1.0, mix), 1e-9);
    }
}

TEST(2, "the result does not depend on the scheduling") {
    const std::size_t n = 300;
    const double expected = full_tableau(n, 2.0, mix);
    for (unsigned workers : {1U, 2U, 3U, 4U}) {
        INFO("workers=", workers);
        practice::set_workers(workers);
        PlainArray<double> a(2 * n - 1);
        CHECK_NEAR(parallel_tableau(a, n, 2.0, mix), expected, 1e-9);
    }
}

TEST(2, "only the diagonal slots are used") {
    practice::set_workers(2);
    const std::size_t n = 128;
    CheckedArray a(2 * n - 1);
    CHECK_NEAR(parallel_tableau(a, n, 1.5, mix), full_tableau(n, 1.5, mix), 1e-9);
}

TEST(3, "work and span of the single-level split") {
    const auto four = tableau_work_span(2, 2);
    CHECK_EQ(four.work, 4);
    CHECK_EQ(four.span, 3);  // top-left, then two in parallel, then bottom-right
    const auto nine = tableau_work_span(3, 3);
    CHECK_EQ(nine.work, 9);
    CHECK_EQ(nine.span, 5);
}

TEST(3, "work counts every cell at any depth") {
    for (const auto& [n, k] : std::vector<std::pair<std::size_t, std::size_t>>{{8, 2}, {27, 3}, {16, 4}, {1, 2}}) {
        INFO("n=", n, " k=", k);
        CHECK_EQ(tableau_work_span(n, k).work, n * n);
    }
}

TEST(3, "span follows the recursion over the anti-diagonals") {
    for (const auto& [n, k] : std::vector<std::pair<std::size_t, std::size_t>>{{4, 2}, {32, 2}, {9, 3}, {81, 3}, {16, 4}, {1, 2}}) {
        INFO("n=", n, " k=", k);
        CHECK_EQ(tableau_work_span(n, k).span, span_by_recursion(n, k));
    }
}

TEST(3, "span grows like n to the log base k of 2k minus 1") {
    const auto small = tableau_work_span(64, 4);
    const auto large = tableau_work_span(256, 4);
    const double measured = std::log(static_cast<double>(large.span) / static_cast<double>(small.span)) /
                            std::log(4.0);
    CHECK_NEAR(measured, std::log(7.0) / std::log(4.0), 1e-9);
}
