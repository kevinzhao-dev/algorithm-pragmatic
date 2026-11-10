#include <cmath>
#include <numeric>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

TEST(1, "fits an exact line") {
    const std::vector<double> x{1, 2, 3, 4};
    const std::vector<double> y{3, 5, 7, 9};
    const LineFit fit = fit_line(x, y);
    CHECK_NEAR(fit.slope, 2.0, 1e-9);
    CHECK_NEAR(fit.intercept, 1.0, 1e-9);
    CHECK_NEAR(fit.r_squared, 1.0, 1e-9);
}

TEST(1, "two points always fit perfectly") {
    const std::vector<double> x{0, 10};
    const std::vector<double> y{4, 1};
    const LineFit fit = fit_line(x, y);
    CHECK_NEAR(fit.slope, -0.3, 1e-9);
    CHECK_NEAR(fit.intercept, 4.0, 1e-9);
    CHECK_NEAR(fit.r_squared, 1.0, 1e-9);
}

TEST(1, "constant y has slope zero and r_squared one") {
    const std::vector<double> x{1, 2, 3, 4, 5};
    const std::vector<double> y{7, 7, 7, 7, 7};
    const LineFit fit = fit_line(x, y);
    CHECK_NEAR(fit.slope, 0.0, 1e-9);
    CHECK_NEAR(fit.intercept, 7.0, 1e-9);
    CHECK_NEAR(fit.r_squared, 1.0, 1e-9);
}

TEST(1, "invalid inputs are rejected") {
    const std::vector<double> one{1};
    const std::vector<double> two_a{1, 2};
    const std::vector<double> two_b{3, 4, 5};
    const std::vector<double> equal{2, 2, 2};
    CHECK_THROWS_AS(fit_line(one, one), std::invalid_argument);
    CHECK_THROWS_AS(fit_line(two_a, two_b), std::invalid_argument);
    CHECK_THROWS_AS(fit_line(equal, two_b), std::invalid_argument);
}

TEST(1, "residuals are orthogonal to x for noisy data") {
    Rng rng;
    for (int trial = 0; trial < 50; ++trial) {
        INFO("trial ", trial);
        const auto n = static_cast<std::size_t>(rng.uniform(3, 200));
        std::vector<double> x(n), y(n);
        for (std::size_t i = 0; i < n; ++i) {
            x[i] = rng.real() * 100;
            y[i] = 3 * x[i] - 7 + (rng.real() - 0.5) * 20;
        }
        const LineFit fit = fit_line(x, y);
        double sum_residual = 0, sum_residual_x = 0;
        for (std::size_t i = 0; i < n; ++i) {
            const double r = y[i] - (fit.slope * x[i] + fit.intercept);
            sum_residual += r;
            sum_residual_x += r * x[i];
        }
        CHECK_NEAR(sum_residual, 0.0, 1e-6);
        CHECK_NEAR(sum_residual_x, 0.0, 1e-4);
        CHECK_LE(fit.r_squared, 1.0 + 1e-12);
    }
}

TEST(2, "pearson sees perfect correlation in both directions") {
    const std::vector<double> x{1, 2, 3, 4};
    const std::vector<double> up{2, 4, 6, 8};
    const std::vector<double> down{8, 6, 4, 2};
    CHECK_NEAR(pearson(x, up), 1.0, 1e-9);
    CHECK_NEAR(pearson(x, down), -1.0, 1e-9);
}

TEST(2, "pearson is zero for constant series and for symmetric data") {
    const std::vector<double> x{1, 2, 3};
    const std::vector<double> constant{5, 5, 5};
    CHECK_NEAR(pearson(x, constant), 0.0, 1e-12);
    const std::vector<double> v{-1, 1, -1, 1};
    const std::vector<double> w{-1, -1, 1, 1};
    CHECK_NEAR(pearson(v, w), 0.0, 1e-12);
}

TEST(2, "pearson rejects invalid input") {
    const std::vector<double> a{1, 2};
    const std::vector<double> b{1};
    CHECK_THROWS_AS(pearson(a, b), std::invalid_argument);
    CHECK_THROWS_AS(pearson(b, b), std::invalid_argument);
}

TEST(2, "two regressors are recovered exactly") {
    const std::vector<std::vector<double>> rows{{1, 2}, {3, 1}, {2, 5}, {4, 3}};
    std::vector<double> y;
    for (const auto& row : rows) {
        y.push_back(3.5 * row[0] - 2.0 * row[1]);
    }
    const auto beta = fit_multilinear(rows, y);
    REQUIRE_EQ(beta.size(), 2);
    CHECK_NEAR(beta[0], 3.5, 1e-9);
    CHECK_NEAR(beta[1], -2.0, 1e-9);
}

TEST(2, "regressors of very different magnitude are handled") {
    Rng rng;
    std::vector<std::vector<double>> rows;
    std::vector<double> y;
    for (int i = 0; i < 40; ++i) {
        const double instructions = 1e8 + rng.real() * 9e8;
        const double calls = 1e3 + rng.real() * 9e3;
        rows.push_back({instructions, calls});
        y.push_back(0.3 * instructions + 7.0 * calls);
    }
    const auto beta = fit_multilinear(rows, y);
    CHECK_NEAR(beta[0], 0.3, 1e-6);
    CHECK_NEAR(beta[1], 7.0, 1e-3);
}

TEST(2, "linearly dependent regressors are rejected") {
    const std::vector<std::vector<double>> rows{{1, 2}, {2, 4}, {3, 6}, {5, 10}};
    const std::vector<double> y{1, 2, 3, 5};
    CHECK_THROWS_AS(fit_multilinear(rows, y), std::domain_error);
}

TEST(2, "a zero column is rejected and malformed rows are invalid") {
    const std::vector<std::vector<double>> zero{{1, 0}, {2, 0}, {3, 0}};
    const std::vector<double> y{1, 2, 3};
    CHECK_THROWS_AS(fit_multilinear(zero, y), std::domain_error);
    const std::vector<std::vector<double>> jagged{{1, 2}, {3}};
    const std::vector<double> y2{1, 2};
    CHECK_THROWS_AS(fit_multilinear(jagged, y2), std::invalid_argument);
    const std::vector<double> short_y{1};
    const std::vector<std::vector<double>> two{{1}, {2}};
    CHECK_THROWS_AS(fit_multilinear(two, short_y), std::invalid_argument);
}

TEST(2, "noisy fits satisfy the normal equations") {
    Rng rng;
    std::vector<std::vector<double>> rows;
    std::vector<double> y;
    for (int i = 0; i < 120; ++i) {
        const double a = rng.real() * 50, b = rng.real() * 50, c = rng.real() * 50;
        rows.push_back({a, b, c});
        y.push_back(2 * a + 3 * b - c + (rng.real() - 0.5) * 8);
    }
    const auto beta = fit_multilinear(rows, y);
    REQUIRE_EQ(beta.size(), 3);
    for (std::size_t col = 0; col < 3; ++col) {
        INFO("column ", col);
        double inner = 0;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            double predicted = 0;
            for (std::size_t j = 0; j < 3; ++j) {
                predicted += beta[j] * rows[i][j];
            }
            inner += (y[i] - predicted) * rows[i][col];
        }
        CHECK_NEAR(inner, 0.0, 1e-5);
    }
    CHECK_NEAR(beta[0], 2.0, 0.2);
    CHECK_NEAR(beta[1], 3.0, 0.2);
    CHECK_NEAR(beta[2], -1.0, 0.2);
}

namespace {

std::vector<double> doubling_sizes(int low, int high) {
    std::vector<double> sizes;
    for (int k = low; k <= high; ++k) {
        sizes.push_back(std::pow(2.0, k));
    }
    return sizes;
}

template <typename F>
std::vector<double> apply(const std::vector<double>& sizes, F f) {
    std::vector<double> out;
    for (const double n : sizes) {
        out.push_back(f(n));
    }
    return out;
}

}  // namespace

TEST(3, "power law exponent reads back the exponent") {
    const auto sizes = doubling_sizes(3, 12);
    CHECK_NEAR(power_law_exponent(sizes, apply(sizes, [](double n) { return 5.0 * n; })), 1.0, 1e-9);
    CHECK_NEAR(power_law_exponent(sizes, apply(sizes, [](double n) { return 2.0 * n * n; })), 2.0, 1e-9);
    CHECK_NEAR(power_law_exponent(sizes, apply(sizes, [](double n) { return n * n * n; })), 3.0, 1e-9);
}

TEST(3, "every growth class is recognised from exact counts") {
    const auto sizes = doubling_sizes(4, 12);
    CHECK(classify_growth(sizes, apply(sizes, [](double) { return 42.0; })) == Growth::Constant);
    CHECK(classify_growth(sizes, apply(sizes, [](double n) { return 3.0 * std::log2(n); })) ==
          Growth::Logarithmic);
    CHECK(classify_growth(sizes, apply(sizes, [](double n) { return 7.0 * n; })) == Growth::Linear);
    CHECK(classify_growth(sizes, apply(sizes, [](double n) { return 2.0 * n * std::log2(n); })) ==
          Growth::NLogN);
    CHECK(classify_growth(sizes, apply(sizes, [](double n) { return 4.0 * n * n; })) ==
          Growth::Quadratic);
    CHECK(classify_growth(sizes, apply(sizes, [](double n) { return 0.5 * n * n * n; })) ==
          Growth::Cubic);
}

TEST(3, "exponential growth is recognised on a linear size range") {
    std::vector<double> n, counts;
    for (int i = 0; i <= 18; ++i) {
        n.push_back(static_cast<double>(i + 10));
        counts.push_back(3.0 * std::pow(1.618, i + 10));
    }
    CHECK(classify_growth(n, counts) == Growth::Exponential);
}

TEST(3, "classes survive ten percent multiplicative noise") {
    Rng rng;
    const auto sizes = doubling_sizes(5, 14);
    const auto noisy = [&](double value) { return value * (0.9 + 0.2 * rng.real()); };
    CHECK(classify_growth(sizes, apply(sizes, [&](double n) { return noisy(8.0 * n); })) ==
          Growth::Linear);
    CHECK(classify_growth(sizes, apply(sizes, [&](double n) { return noisy(3.0 * n * n); })) ==
          Growth::Quadratic);
    CHECK(classify_growth(sizes, apply(sizes, [&](double n) { return noisy(n * n * n); })) ==
          Growth::Cubic);
}

TEST(3, "the real counts of an algorithm are classified") {
    // Calls made by a binary recursion over [lo, hi): exactly 2n - 1.
    const auto sizes = doubling_sizes(3, 12);
    CHECK(classify_growth(sizes, apply(sizes, [](double n) { return 2.0 * n - 1.0; })) ==
          Growth::Linear);
    // Nested loops with n(n - 1)/2 inner iterations.
    CHECK(classify_growth(sizes, apply(sizes, [](double n) { return n * (n - 1) / 2; })) ==
          Growth::Quadratic);
}

TEST(3, "too few samples are rejected") {
    const std::vector<double> two{1, 2};
    CHECK_THROWS_AS(classify_growth(two, two), std::invalid_argument);
    const std::vector<double> three{1, 2, 3};
    const std::vector<double> mismatch{1, 2};
    CHECK_THROWS_AS(classify_growth(three, mismatch), std::invalid_argument);
    CHECK_THROWS_AS(power_law_exponent(three, mismatch), std::invalid_argument);
}
