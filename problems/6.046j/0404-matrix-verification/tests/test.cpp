#include <cstdint>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Mat = Matrix<std::uint64_t>;

namespace {

// A ring element that counts multiplications, so complexity can be asserted exactly.
class CountingRing {
public:
    CountingRing() = default;
    explicit CountingRing(std::uint64_t value) : value_(value) {}

    friend CountingRing operator+(CountingRing a, CountingRing b) { return CountingRing(a.value_ + b.value_); }
    friend CountingRing operator-(CountingRing a, CountingRing b) { return CountingRing(a.value_ - b.value_); }
    friend CountingRing operator*(CountingRing a, CountingRing b) {
        ++multiplications();
        return CountingRing(a.value_ * b.value_);
    }
    friend bool operator==(CountingRing, CountingRing) = default;

    static std::uint64_t& multiplications() {
        static std::uint64_t count = 0;
        return count;
    }

private:
    std::uint64_t value_ = 0;
};

template <typename Ring = std::uint64_t>
Matrix<Ring> random_matrix(Rng& rng, std::size_t n) {
    Matrix<Ring> m(n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            m(i, j) = Ring(rng.engine()());
        }
    }
    return m;
}

Mat identity(std::size_t n) {
    Mat m(n);
    for (std::size_t i = 0; i < n; ++i) {
        m(i, i) = 1;
    }
    return m;
}

std::uint64_t power(std::uint64_t base, int exponent) {
    std::uint64_t result = 1;
    for (int i = 0; i < exponent; ++i) {
        result *= base;
    }
    return result;
}

}  // namespace

TEST(1, "a new matrix is all zeros") {
    const Mat m(3);
    CHECK_EQ(m.size(), 3);
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < 3; ++j) {
            CHECK_EQ(m(i, j), 0);
        }
    }
}

TEST(1, "cells are independent and writable") {
    Mat m(2);
    m(0, 1) = 5;
    m(1, 0) = 7;
    CHECK_EQ(m(0, 1), 5);
    CHECK_EQ(m(1, 0), 7);
    CHECK_EQ(m(0, 0), 0);
}

TEST(1, "matrices compare by value") {
    Mat a(2);
    Mat b(2);
    CHECK(a == b);
    b(1, 1) = 1;
    CHECK(!(a == b));
    Mat c(3);
    CHECK(!(a == c));
}

TEST(1, "a product of known small matrices") {
    Mat a(2);
    a(0, 0) = 1;
    a(0, 1) = 2;
    a(1, 0) = 3;
    a(1, 1) = 4;
    Mat b(2);
    b(0, 0) = 5;
    b(0, 1) = 6;
    b(1, 0) = 7;
    b(1, 1) = 8;
    const Mat c = multiply_naive(a, b);
    CHECK_EQ(c(0, 0), 19);
    CHECK_EQ(c(0, 1), 22);
    CHECK_EQ(c(1, 0), 43);
    CHECK_EQ(c(1, 1), 50);
}

TEST(1, "multiplying by the identity changes nothing") {
    Rng rng;
    const Mat a = random_matrix(rng, 5);
    CHECK(multiply_naive(a, identity(5)) == a);
    CHECK(multiply_naive(identity(5), a) == a);
}

TEST(1, "mismatched sizes are rejected") {
    CHECK_THROWS_AS(multiply_naive(Mat(2), Mat(3)), std::invalid_argument);
}

TEST(1, "arithmetic wraps around like unsigned 64-bit integers") {
    Mat a(1);
    a(0, 0) = 0xFFFFFFFFFFFFFFFFULL;
    Mat b(1);
    b(0, 0) = 2;
    CHECK_EQ(multiply_naive(a, b)(0, 0), 0xFFFFFFFFFFFFFFFEULL);
}

TEST(1, "the naive algorithm performs n cubed multiplications") {
    Rng rng;
    const auto a = random_matrix<CountingRing>(rng, 16);
    const auto b = random_matrix<CountingRing>(rng, 16);
    CountingRing::multiplications() = 0;
    multiply_naive(a, b);
    CHECK_LE(CountingRing::multiplications(), 16 * 16 * 16);
}

TEST(2, "a cutoff of zero is rejected") {
    CHECK_THROWS_AS(multiply_strassen(Mat(2), Mat(2), 0), std::invalid_argument);
}

TEST(2, "mismatched sizes are rejected") {
    CHECK_THROWS_AS(multiply_strassen(Mat(2), Mat(3)), std::invalid_argument);
}

TEST(2, "an empty matrix stays empty") {
    CHECK_EQ(multiply_strassen(Mat(0), Mat(0)).size(), 0);
}

TEST(2, "a single cell multiplies directly") {
    Mat a(1);
    a(0, 0) = 3;
    Mat b(1);
    b(0, 0) = 4;
    CHECK_EQ(multiply_strassen(a, b, 1)(0, 0), 12);
}

TEST(2, "random power-of-two sizes match the naive product") {
    Rng rng;
    for (std::size_t n : {2u, 4u, 8u, 16u, 32u}) {
        for (std::size_t cutoff : {1u, 2u, 8u}) {
            INFO("n ", n, " cutoff ", cutoff);
            const Mat a = random_matrix(rng, n);
            const Mat b = random_matrix(rng, n);
            CHECK(multiply_strassen(a, b, cutoff) == multiply_naive(a, b));
        }
    }
}

TEST(2, "sizes that are not powers of two are padded and trimmed") {
    Rng rng;
    for (std::size_t n : {3u, 5u, 6u, 7u, 13u, 21u}) {
        INFO("n ", n);
        const Mat a = random_matrix(rng, n);
        const Mat b = random_matrix(rng, n);
        const Mat c = multiply_strassen(a, b, 2);
        CHECK_EQ(c.size(), n);
        CHECK(c == multiply_naive(a, b));
    }
}

TEST(2, "exactly seven recursive products are used per level") {
    Rng rng;
    for (int levels : {1, 2, 3, 4}) {
        const std::size_t n = std::size_t{1} << levels;
        const auto a = random_matrix<CountingRing>(rng, n);
        const auto b = random_matrix<CountingRing>(rng, n);
        CountingRing::multiplications() = 0;
        multiply_strassen(a, b, 1);
        CHECK_EQ(CountingRing::multiplications(), power(7, levels));
    }
}

TEST(2, "the cutoff switches to the naive algorithm at the requested size") {
    Rng rng;
    const std::size_t n = 32;
    const auto a = random_matrix<CountingRing>(rng, n);
    const auto b = random_matrix<CountingRing>(rng, n);
    CountingRing::multiplications() = 0;
    multiply_strassen(a, b, 8);
    CHECK_EQ(CountingRing::multiplications(), power(7, 2) * 8 * 8 * 8);
}

TEST(3, "mismatched sizes are rejected") {
    Rng rng;
    CHECK_THROWS_AS(verify_product(Mat(2), Mat(2), Mat(3), rng.engine()), std::invalid_argument);
    CHECK_THROWS_AS(verify_product(Mat(3), Mat(2), Mat(2), rng.engine()), std::invalid_argument);
}

TEST(3, "a correct product is always accepted") {
    Rng rng;
    for (int trial = 0; trial < 200; ++trial) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 12));
        const Mat a = random_matrix(rng, n);
        const Mat b = random_matrix(rng, n);
        CHECK(verify_product(a, b, multiply_naive(a, b), rng.engine(), 8));
    }
}

TEST(3, "an empty product is accepted") {
    Rng rng;
    CHECK(verify_product(Mat(0), Mat(0), Mat(0), rng.engine()));
}

TEST(3, "a single wrong cell is caught with high probability") {
    Rng rng;
    const std::size_t n = 10;
    const Mat a = random_matrix(rng, n);
    const Mat b = random_matrix(rng, n);
    Mat c = multiply_naive(a, b);
    c(n / 2, n / 3) += 1;
    int caught = 0;
    for (int trial = 0; trial < 500; ++trial) {
        caught += !verify_product(a, b, c, rng.engine(), 1);
    }
    CHECK_GE(caught, 200);
    CHECK(!verify_product(a, b, c, rng.engine(), 40));
}

TEST(3, "random corrupted products are rejected") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 10));
        const Mat a = random_matrix(rng, n);
        const Mat b = random_matrix(rng, n);
        Mat c = multiply_naive(a, b);
        const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
        const auto j = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
        c(i, j) += static_cast<std::uint64_t>(rng.uniform(1, 1'000));
        CHECK(!verify_product(a, b, c, rng.engine(), 40));
    }
}

TEST(3, "verification performs a few matrix-vector products per round") {
    Rng rng;
    const std::size_t n = 32;
    const int rounds = 5;
    const auto a = random_matrix<CountingRing>(rng, n);
    const auto b = random_matrix<CountingRing>(rng, n);
    const auto c = random_matrix<CountingRing>(rng, n);
    CountingRing::multiplications() = 0;
    verify_product(a, b, c, rng.engine(), rounds);
    CHECK_LE(CountingRing::multiplications(), static_cast<std::uint64_t>(3 * rounds) * n * n);
}

PERF(3, "checking a product of two 3000 by 3000 matrices", 1500) {
    Rng rng;
    const std::size_t n = 3000;
    const Mat a = random_matrix(rng, n);
    const Mat b = random_matrix(rng, n);
    // The vector check only needs the product applied to r, so feed it a known wrong matrix.
    Mat c = random_matrix(rng, n);
    CHECK(!verify_product(a, b, c, rng.engine(), 20));
}
