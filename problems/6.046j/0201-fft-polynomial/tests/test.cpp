#include <cmath>
#include <numbers>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Poly = std::vector<std::int64_t>;

namespace {

constexpr std::uint64_t kCheckPrime = (std::uint64_t{1} << 61) - 1;

std::uint64_t mul_mod_check(std::uint64_t a, std::uint64_t b) {
    return static_cast<std::uint64_t>(static_cast<unsigned __int128>(a) * b % kCheckPrime);
}

std::uint64_t reduce(std::int64_t value) {
    const std::int64_t prime = static_cast<std::int64_t>(kCheckPrime);
    const std::int64_t r = value % prime;
    return static_cast<std::uint64_t>(r < 0 ? r + prime : r);
}

template <typename Value>
std::uint64_t evaluate(std::span<const Value> poly, std::uint64_t point, std::uint64_t (*to_field)(Value)) {
    std::uint64_t result = 0;
    for (std::size_t i = poly.size(); i-- > 0;) {
        result = (mul_mod_check(result, point) + to_field(poly[i])) % kCheckPrime;
    }
    return result;
}

std::uint64_t field_of_int(std::int64_t value) { return reduce(value); }

// Schwartz-Zippel: a wrong coefficient survives this only with probability below 2^-40 here.
void check_product(const Poly& a, const Poly& b, const Poly& product, Rng& rng) {
    REQUIRE_EQ(product.size(), a.size() + b.size() - 1);
    for (int round = 0; round < 2; ++round) {
        const auto point = static_cast<std::uint64_t>(rng.uniform(2, static_cast<std::int64_t>(kCheckPrime) - 2));
        CHECK_EQ(evaluate<std::int64_t>(product, point, field_of_int),
                 mul_mod_check(evaluate<std::int64_t>(a, point, field_of_int),
                               evaluate<std::int64_t>(b, point, field_of_int)));
    }
}

Poly random_poly(Rng& rng, std::size_t n, std::int64_t amplitude) {
    Poly poly(n);
    for (std::int64_t& value : poly) {
        value = rng.uniform(-amplitude, amplitude);
    }
    return poly;
}

// Every coefficient at full magnitude with random signs: the hardest case for rounding.
Poly random_signs(Rng& rng, std::size_t n, std::int64_t amplitude) {
    Poly poly(n);
    for (std::int64_t& value : poly) {
        value = rng.coin() ? amplitude : -amplitude;
    }
    return poly;
}

std::vector<Complex> naive_dft(const std::vector<Complex>& a) {
    const std::size_t n = a.size();
    std::vector<Complex> result(n);
    for (std::size_t k = 0; k < n; ++k) {
        for (std::size_t j = 0; j < n; ++j) {
            const double angle = 2.0 * std::numbers::pi * static_cast<double>(j * k) / static_cast<double>(n);
            result[k] += a[j] * Complex(std::cos(angle), std::sin(angle));
        }
    }
    return result;
}

void check_close(const std::vector<Complex>& a, const std::vector<Complex>& b, double tolerance) {
    REQUIRE_EQ(a.size(), b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        INFO("index ", i);
        CHECK_NEAR(a[i].real(), b[i].real(), tolerance);
        CHECK_NEAR(a[i].imag(), b[i].imag(), tolerance);
    }
}

std::uint32_t mul_ntt(std::uint32_t a, std::uint32_t b) {
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(a) * b % kNttModulus);
}

std::vector<std::uint32_t> naive_mod(const std::vector<std::uint32_t>& a,
                                     const std::vector<std::uint32_t>& b) {
    if (a.empty() || b.empty()) {
        return {};
    }
    std::vector<std::uint32_t> product(a.size() + b.size() - 1, 0);
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (std::size_t j = 0; j < b.size(); ++j) {
            product[i + j] = (product[i + j] + mul_ntt(a[i], b[j])) % kNttModulus;
        }
    }
    return product;
}

std::uint32_t eval_ntt(const std::vector<std::uint32_t>& poly, std::uint32_t point) {
    std::uint32_t result = 0;
    for (std::size_t i = poly.size(); i-- > 0;) {
        result = (mul_ntt(result, point) + poly[i]) % kNttModulus;
    }
    return result;
}

std::uint64_t decimal_mod(std::string_view text, std::uint64_t modulus) {
    std::uint64_t value = 0;
    for (char c : text) {
        value = (value * 10 + static_cast<std::uint64_t>(c - '0')) % modulus;
    }
    return value;
}

std::string random_digits(Rng& rng, std::size_t n) {
    std::string digits = rng.text(n, "0123456789");
    digits[0] = static_cast<char>('1' + rng.uniform(0, 8));
    return digits;
}

std::string to_decimal(__int128 value) {
    if (value == 0) {
        return "0";
    }
    const bool negative = value < 0;
    unsigned __int128 magnitude = negative ? -static_cast<unsigned __int128>(value) : static_cast<unsigned __int128>(value);
    std::string out;
    while (magnitude != 0) {
        out.insert(out.begin(), static_cast<char>('0' + static_cast<int>(magnitude % 10)));
        magnitude /= 10;
    }
    return negative ? "-" + out : out;
}

}  // namespace

TEST(1, "multiplying by an empty polynomial gives an empty result") {
    const Poly one{1};
    CHECK(multiply_naive({}, one).empty());
    CHECK(multiply_naive(one, {}).empty());
}

TEST(1, "two linear polynomials") {
    CHECK_EQ(multiply_naive(Poly{1, 2}, Poly{3, 4}), (Poly{3, 10, 8}));
}

TEST(1, "signs and zeros are preserved") {
    CHECK_EQ(multiply_naive(Poly{0, 1, -1}, Poly{2, 0}), (Poly{0, 2, -2, 0}));
}

TEST(2, "a non power-of-two size is rejected") {
    std::vector<Complex> values(3);
    CHECK_THROWS_AS(fft(values), std::invalid_argument);
}

TEST(2, "the empty span is rejected") {
    std::vector<Complex> values;
    CHECK_THROWS_AS(fft(values), std::invalid_argument);
}

TEST(2, "a single sample is its own transform") {
    std::vector<Complex> values{Complex(3.5, -1.0)};
    fft(values);
    CHECK_NEAR(values[0].real(), 3.5, 1e-12);
    CHECK_NEAR(values[0].imag(), -1.0, 1e-12);
}

TEST(2, "the forward transform uses the positive exponent") {
    // A(x) = x sampled at the four roots of unity: 1, i, -1, -i.
    std::vector<Complex> values{0.0, 1.0, 0.0, 0.0};
    fft(values);
    check_close(values, {Complex(1, 0), Complex(0, 1), Complex(-1, 0), Complex(0, -1)}, 1e-12);
}

TEST(2, "random inputs match the direct definition") {
    Rng rng;
    for (std::size_t n : {1u, 2u, 4u, 8u, 16u, 64u, 256u, 1024u}) {
        std::vector<Complex> values(n);
        for (Complex& value : values) {
            value = Complex(rng.real() - 0.5, rng.real() - 0.5);
        }
        const auto expected = naive_dft(values);
        fft(values);
        check_close(values, expected, 1e-9);
    }
}

TEST(2, "the inverse undoes the forward transform") {
    Rng rng;
    for (std::size_t n : {1u, 2u, 32u, 4096u}) {
        std::vector<Complex> values(n);
        for (Complex& value : values) {
            value = Complex(rng.real(), rng.real());
        }
        const auto original = values;
        fft(values);
        fft(values, true);
        check_close(values, original, 1e-9);
    }
}

PERF(2, "a transform of one million points", 1200) {
    Rng rng;
    std::vector<Complex> values(std::size_t{1} << 20);
    for (Complex& value : values) {
        value = Complex(rng.real(), rng.real());
    }
    const auto original = values;
    fft(values);
    fft(values, true);
    check_close(values, original, 1e-6);
}

TEST(3, "multiplying with empty input is empty") {
    const Poly one{1};
    CHECK(multiply({}, one).empty());
    CHECK(multiply(one, {}).empty());
}

TEST(3, "a constant times a polynomial") {
    CHECK_EQ(multiply(Poly{5}, Poly{1, 2, 3}), (Poly{5, 10, 15}));
}

TEST(3, "the result keeps trailing zeros") {
    CHECK_EQ(multiply(Poly{1, 0}, Poly{1, 0, 0}), (Poly{1, 0, 0, 0}));
}

TEST(3, "signs are rounded to the correct integer") {
    CHECK_EQ(multiply(Poly{-3, 1}, Poly{-1, -2}), (Poly{3, 5, -2}));
}

TEST(3, "the zero polynomial stays zero") {
    CHECK_EQ(multiply(Poly{0, 0, 0}, Poly{7, 7}), (Poly{0, 0, 0, 0}));
}

TEST(3, "random polynomials match the direct product") {
    Rng rng;
    for (int trial = 0; trial < 400; ++trial) {
        INFO("trial ", trial);
        const auto a = random_poly(rng, static_cast<std::size_t>(rng.uniform(1, 200)), 1000);
        const auto b = random_poly(rng, static_cast<std::size_t>(rng.uniform(1, 200)), 1000);
        CHECK_EQ(multiply(a, b), multiply_naive(a, b));
    }
}

TEST(3, "constant polynomials hit the worst magnitude exactly") {
    const std::size_t n = std::size_t{1} << 19;
    const Poly a(n, 2048);
    const Poly b(n, 2048);
    const Poly product = multiply(a, b);
    REQUIRE_EQ(product.size(), 2 * n - 1);
    for (std::size_t k = 0; k < product.size(); ++k) {
        const std::int64_t terms = static_cast<std::int64_t>(std::min(k + 1, 2 * n - 1 - k));
        REQUIRE_EQ(product[k], 2048 * 2048 * terms);
    }
}

TEST(3, "random signs at the contract limit round exactly") {
    Rng rng;
    const std::size_t n = std::size_t{1} << 19;
    const auto a = random_signs(rng, n, 2048);
    const auto b = random_signs(rng, n, 2048);
    check_product(a, b, multiply(a, b), rng);
}

TEST(3, "unbalanced lengths at the contract limit round exactly") {
    Rng rng;
    const auto a = random_signs(rng, 1'000, 2048);
    const auto b = random_signs(rng, (std::size_t{1} << 20) - 1'000, 2048);
    check_product(a, b, multiply(a, b), rng);
}

TEST(3, "inputs past the contract are rejected instead of rounded wrongly") {
    const std::size_t n = std::size_t{1} << 19;
    const Poly a(n, 2049);
    const Poly b(n, 2048);
    CHECK_THROWS_AS(multiply(a, b), std::domain_error);
    const Poly huge{4'000'000'000'000'000'000};
    CHECK_THROWS_AS(multiply(huge, huge), std::domain_error);
}

PERF(3, "multiplying two polynomials of a million terms", 2000) {
    Rng rng;
    const auto a = random_poly(rng, 1'000'000, 2);
    const auto b = random_poly(rng, 1'000'000, 2);
    check_product(a, b, multiply(a, b), rng);
}

TEST(4, "empty input is empty") {
    const std::vector<std::uint32_t> one{1};
    CHECK(multiply_mod({}, one).empty());
    CHECK(multiply_mod(one, {}).empty());
}

TEST(4, "a single term") {
    CHECK_EQ(multiply_mod(std::vector<std::uint32_t>{7}, std::vector<std::uint32_t>{9}),
             std::vector<std::uint32_t>{63});
}

TEST(4, "coefficients wrap around the modulus") {
    const std::vector<std::uint32_t> a{kNttModulus - 1, 2};
    const std::vector<std::uint32_t> b{kNttModulus - 1, 3};
    CHECK_EQ(multiply_mod(a, b), naive_mod(a, b));
}

TEST(4, "unreduced coefficients are rejected") {
    const std::vector<std::uint32_t> a{kNttModulus};
    const std::vector<std::uint32_t> b{1};
    CHECK_THROWS_AS(multiply_mod(a, b), std::invalid_argument);
}

TEST(4, "a product longer than the modulus supports is rejected") {
    const std::vector<std::uint32_t> a((std::size_t{1} << 23) + 1, 0);
    const std::vector<std::uint32_t> b{1};
    CHECK_THROWS_AS(multiply_mod(a, b), std::length_error);
}

TEST(4, "random polynomials match the direct product") {
    Rng rng;
    for (int trial = 0; trial < 400; ++trial) {
        INFO("trial ", trial);
        std::vector<std::uint32_t> a(static_cast<std::size_t>(rng.uniform(1, 200)));
        std::vector<std::uint32_t> b(static_cast<std::size_t>(rng.uniform(1, 200)));
        for (auto& value : a) {
            value = static_cast<std::uint32_t>(rng.uniform(0, kNttModulus - 1));
        }
        for (auto& value : b) {
            value = static_cast<std::uint32_t>(rng.uniform(0, kNttModulus - 1));
        }
        CHECK_EQ(multiply_mod(a, b), naive_mod(a, b));
    }
}

TEST(4, "exact even where floating point could not round") {
    Rng rng;
    std::vector<std::uint32_t> a(std::size_t{1} << 18);
    std::vector<std::uint32_t> b(std::size_t{1} << 18);
    for (auto& value : a) {
        value = kNttModulus - 1;
    }
    for (auto& value : b) {
        value = static_cast<std::uint32_t>(rng.uniform(0, kNttModulus - 1));
    }
    const auto product = multiply_mod(a, b);
    REQUIRE_EQ(product.size(), a.size() + b.size() - 1);
    for (int round = 0; round < 3; ++round) {
        const auto point = static_cast<std::uint32_t>(rng.uniform(2, kNttModulus - 2));
        CHECK_EQ(eval_ntt(product, point), mul_ntt(eval_ntt(a, point), eval_ntt(b, point)));
    }
}

PERF(4, "a transform-based product of a million terms", 2000) {
    Rng rng;
    std::vector<std::uint32_t> a(1'000'000);
    std::vector<std::uint32_t> b(1'000'000);
    for (std::size_t i = 0; i < a.size(); ++i) {
        a[i] = static_cast<std::uint32_t>(rng.uniform(0, kNttModulus - 1));
        b[i] = static_cast<std::uint32_t>(rng.uniform(0, kNttModulus - 1));
    }
    const auto product = multiply_mod(a, b);
    const std::uint32_t point = 12345;
    CHECK_EQ(eval_ntt(product, point), mul_ntt(eval_ntt(a, point), eval_ntt(b, point)));
}

TEST(5, "small products") {
    CHECK_EQ(multiply_decimal("12", "34"), "408");
    CHECK_EQ(multiply_decimal("99", "99"), "9801");
    CHECK_EQ(multiply_decimal("1", "1"), "1");
}

TEST(5, "zero never carries a sign or extra digits") {
    CHECK_EQ(multiply_decimal("0", "123"), "0");
    CHECK_EQ(multiply_decimal("-5", "0"), "0");
    CHECK_EQ(multiply_decimal("000", "0"), "0");
}

TEST(5, "leading zeros are normalized") {
    CHECK_EQ(multiply_decimal("007", "00011"), "77");
}

TEST(5, "signs combine") {
    CHECK_EQ(multiply_decimal("-3", "4"), "-12");
    CHECK_EQ(multiply_decimal("-3", "-4"), "12");
}

TEST(5, "malformed numbers are rejected") {
    CHECK_THROWS_AS(multiply_decimal("", "1"), std::invalid_argument);
    CHECK_THROWS_AS(multiply_decimal("-", "1"), std::invalid_argument);
    CHECK_THROWS_AS(multiply_decimal("1", "1a"), std::invalid_argument);
    CHECK_THROWS_AS(multiply_decimal("+1", "1"), std::invalid_argument);
    CHECK_THROWS_AS(multiply_decimal("1-", "1"), std::invalid_argument);
}

TEST(5, "products carry across every limb") {
    CHECK_EQ(multiply_decimal("999999999999999999", "999999999999999999"),
             "999999999999999998000000000000000001");
}

TEST(5, "random values match 128-bit arithmetic") {
    Rng rng;
    for (int trial = 0; trial < 3000; ++trial) {
        INFO("trial ", trial);
        const std::int64_t a = rng.uniform(-999'999'999'999'999'999, 999'999'999'999'999'999);
        const std::int64_t b = rng.uniform(-999'999'999'999'999'999, 999'999'999'999'999'999);
        CHECK_EQ(multiply_decimal(std::to_string(a), std::to_string(b)),
                 to_decimal(static_cast<__int128>(a) * b));
    }
}

TEST(5, "long numbers agree modulo primes") {
    Rng rng;
    const std::string a = random_digits(rng, 40'000);
    const std::string b = random_digits(rng, 25'000);
    const std::string product = multiply_decimal(a, b);
    CHECK(product.front() != '0');
    CHECK(product.size() == a.size() + b.size() || product.size() + 1 == a.size() + b.size());
    for (std::uint64_t modulus : {1'000'000'007ULL, 998'244'353ULL, 1'000'000'009ULL}) {
        CHECK_EQ(decimal_mod(product, modulus),
                 decimal_mod(a, modulus) * decimal_mod(b, modulus) % modulus);
    }
}

PERF(5, "two numbers of a million digits", 3000) {
    Rng rng;
    const std::string a = random_digits(rng, 1'000'000);
    const std::string b = random_digits(rng, 1'000'000);
    const std::string product = multiply_decimal(a, b);
    for (std::uint64_t modulus : {1'000'000'007ULL, 998'244'353ULL}) {
        CHECK_EQ(decimal_mod(product, modulus),
                 decimal_mod(a, modulus) * decimal_mod(b, modulus) % modulus);
    }
}
