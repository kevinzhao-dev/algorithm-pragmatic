#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Coefficients = std::vector<std::int64_t>;

namespace {

Poly poly(Coefficients coefficients) { return Poly(std::move(coefficients)); }

// Coefficients of q(x) = p(x) - p(x - 1), computed from the binomial expansion of (x - 1)^i
// so this does not depend on the code under test.
Coefficients backward_difference(const Coefficients& p) {
    std::vector<std::vector<std::int64_t>> binomial(p.size() + 1,
                                                    std::vector<std::int64_t>(p.size() + 1, 0));
    for (std::size_t n = 0; n <= p.size(); ++n) {
        binomial[n][0] = 1;
        for (std::size_t k = 1; k <= n; ++k) {
            binomial[n][k] = binomial[n - 1][k - 1] + binomial[n - 1][k];
        }
    }
    Coefficients shifted(p.size(), 0);
    for (std::size_t i = 0; i < p.size(); ++i) {
        for (std::size_t j = 0; j <= i; ++j) {
            const std::int64_t sign = (i - j) % 2 == 0 ? 1 : -1;
            shifted[j] += sign * p[i] * binomial[i][j];
        }
    }
    Coefficients difference = p;
    for (std::size_t i = 0; i < p.size(); ++i) {
        difference[i] -= shifted[i];
    }
    return difference;
}

std::int64_t brute_force_sum(const Poly& term, std::int64_t n) {
    std::int64_t total = 0;
    for (std::int64_t i = 1; i <= n; ++i) {
        total += term(i);
    }
    return total;
}

}  // namespace

TEST(1, "small sums of cubes") {
    CHECK_EQ(checked_sum_of_cubes(0), std::optional<std::uint64_t>(0));
    CHECK_EQ(checked_sum_of_cubes(1), std::optional<std::uint64_t>(1));
    CHECK_EQ(checked_sum_of_cubes(4), std::optional<std::uint64_t>(100));
    std::uint64_t total = 0;
    for (std::uint64_t n = 1; n <= 2'000; ++n) {
        total += n * n * n;
        INFO("n=", n);
        REQUIRE_EQ(checked_sum_of_cubes(n), std::optional<std::uint64_t>(total));
    }
}

TEST(1, "the last n whose sum fits in 64 bits") {
    CHECK_EQ(checked_sum_of_cubes(92'681), std::optional<std::uint64_t>(18'446'425'603'259'108'841ULL));
}

TEST(1, "beyond that the result is nullopt, however large n gets") {
    CHECK(!checked_sum_of_cubes(92'682).has_value());
    CHECK(!checked_sum_of_cubes(1ULL << 32).has_value());
    CHECK(!checked_sum_of_cubes(~0ULL).has_value());
}

TEST(1, "sum_of_cubes_mod matches a loop for small n") {
    for (std::uint64_t n = 0; n <= 300; ++n) {
        for (std::uint64_t m = 1; m <= 40; ++m) {
            unsigned __int128 expected = 0;
            for (std::uint64_t i = 1; i <= n; ++i) {
                expected += static_cast<unsigned __int128>(i) * i * i;
            }
            INFO("n=", n, " m=", m);
            REQUIRE_EQ(sum_of_cubes_mod(n, m), static_cast<std::uint64_t>(expected % m));
        }
    }
}

TEST(1, "sum_of_cubes_mod handles huge n and modulus 1") {
    const std::uint64_t n = 4'000'000'000'000'000'000ULL;
    const std::uint64_t m = 1'000'000'007;
    const unsigned __int128 triangular = static_cast<unsigned __int128>(n) * (n + 1) / 2;
    const std::uint64_t reduced = static_cast<std::uint64_t>(triangular % m);
    CHECK_EQ(sum_of_cubes_mod(n, m), reduced * reduced % m);
    CHECK_EQ(sum_of_cubes_mod(n, 1), 0);
}

// A loop over n cannot finish.
PERF(1, "sum_of_cubes_mod is O(1) per call", 200) {
    std::uint64_t sum = 0;
    for (std::uint64_t k = 1; k <= 1'000'000; ++k) {
        sum += sum_of_cubes_mod(4'000'000'000'000'000'000ULL - k, 998'244'353);
    }
    CHECK_GT(sum, 0);
}

TEST(2, "construction drops trailing zeros") {
    CHECK_EQ(poly({1, 2, 0, 0}).coefficients(), Coefficients{1, 2});
    CHECK_EQ(poly({1, 2, 0, 0}).degree(), 1);
    CHECK_EQ(poly({0, 0, 0}).coefficients(), Coefficients{});
    CHECK_EQ(poly({0, 0, 0}).degree(), -1);
    CHECK_EQ(poly({}).degree(), -1);
    CHECK_EQ(Poly{}.degree(), -1);
    CHECK_EQ(poly({7}).degree(), 0);
}

TEST(2, "evaluation uses every coefficient") {
    const Poly p = poly({5, 0, -3, 2});  // 2x^3 - 3x^2 + 5
    CHECK_EQ(p(0), 5);
    CHECK_EQ(p(1), 4);
    CHECK_EQ(p(-1), 0);
    CHECK_EQ(p(2), 9);
    CHECK_EQ(Poly{}(123), 0);
}

TEST(2, "addition, subtraction and cancellation") {
    const Poly a = poly({1, 2, 3});
    const Poly b = poly({4, -2, -3});
    CHECK_EQ((a + b).coefficients(), Coefficients{5});
    CHECK_EQ((a - b).coefficients(), Coefficients{-3, 4, 6});
    CHECK_EQ((a - a).coefficients(), Coefficients{});
    CHECK((a - a) == Poly{});
    CHECK_EQ((a + Poly{}).coefficients(), a.coefficients());
}

TEST(2, "multiplication") {
    const Poly x_plus_one = poly({1, 1});
    CHECK_EQ((x_plus_one * x_plus_one).coefficients(), Coefficients{1, 2, 1});
    CHECK_EQ((poly({2}) * poly({3, 5})).coefficients(), Coefficients{6, 10});
    CHECK_EQ((poly({3, 5}) * Poly{}).coefficients(), Coefficients{});
    CHECK_EQ((poly({-1, 0, 1}) * poly({1, 0, 1})).coefficients(), Coefficients{-1, 0, 0, 0, 1});
}

TEST(2, "equality") {
    CHECK(poly({1, 2}) == poly({1, 2, 0}));
    CHECK(!(poly({1, 2}) == poly({1, 3})));
    CHECK(!(poly({1, 2, 1}) == poly({1, 2})));
    CHECK(Poly{} == poly({0}));
}

TEST(2, "shifted evaluates p(x + 1)") {
    CHECK_EQ(poly({0, 0, 1}).shifted().coefficients(), Coefficients{1, 2, 1});  // x^2
    CHECK_EQ(poly({1}).shifted().coefficients(), Coefficients{1});
    CHECK_EQ(Poly{}.shifted().coefficients(), Coefficients{});
    CHECK_EQ(poly({0, 3}).shifted().coefficients(), Coefficients{3, 3});
}

TEST(2, "random polynomials: operations agree with pointwise evaluation") {
    Rng rng;
    const auto random_poly = [&] {
        Coefficients c(static_cast<std::size_t>(rng.uniform(0, 4)));
        for (auto& value : c) {
            value = rng.uniform(-9, 9);
        }
        return poly(c);
    };
    for (int step = 0; step < 3'000; ++step) {
        const Poly a = random_poly();
        const Poly b = random_poly();
        const std::int64_t x = rng.uniform(-9, 9);
        INFO("step ", step, " x=", x);
        REQUIRE_EQ((a + b)(x), a(x) + b(x));
        REQUIRE_EQ((a - b)(x), a(x) - b(x));
        REQUIRE_EQ((a * b)(x), a(x) * b(x));
        REQUIRE_EQ(a.shifted()(x), a(x + 1));
    }
}

TEST(3, "a true identity is proved: 1 + 2 + ... + n") {
    const auto report = check_sum_formula(poly({0, 1}), poly({0, 1, 1}), 2);
    CHECK(report.base_case);
    CHECK(report.inductive_step);
    CHECK(report.proved());
}

TEST(3, "a true identity with a non-trivial denominator: sum of squares") {
    // n(n+1)(2n+1)/6 = (2n^3 + 3n^2 + n) / 6
    const auto report = check_sum_formula(poly({0, 0, 1}), poly({0, 1, 3, 2}), 6);
    CHECK(report.proved());
}

TEST(3, "constant and odd-number sums") {
    CHECK(check_sum_formula(poly({1}), poly({0, 1}), 1).proved());             // sum of 1 is n
    CHECK(check_sum_formula(poly({-1, 2}), poly({0, 0, 1}), 1).proved());       // odd numbers sum to n^2
}

TEST(3, "a formula that passes the step but fails the base case") {
    // n(n+1)/2 + 1 has the right differences but the wrong starting point.
    const auto report = check_sum_formula(poly({0, 1}), poly({2, 1, 1}), 2);
    CHECK(!report.base_case);
    CHECK(report.inductive_step);
    CHECK(!report.proved());
}

TEST(3, "a formula that passes the base case but fails the step") {
    const auto report = check_sum_formula(poly({0, 1}), poly({0, 1}), 1);  // claims sum of i is n
    CHECK(report.base_case);
    CHECK(!report.inductive_step);
    CHECK(!report.proved());
}

TEST(3, "agreeing on many small n is not a proof") {
    // True formula plus c * (n-1)(n-2)(n-3)(n-4): equal to the real sum for n = 1..4 only.
    const Poly term = poly({0, 1});
    const Poly truth = poly({0, 1, 1});
    const Poly glitch = poly({-1, 1}) * poly({-2, 1}) * poly({-3, 1}) * poly({-4, 1});
    const Poly claim = truth + poly({2}) * glitch;
    for (std::int64_t n = 1; n <= 4; ++n) {
        CHECK_EQ(claim(n), 2 * brute_force_sum(term, n));
    }
    CHECK_NE(claim(5), 2 * brute_force_sum(term, 5));

    const auto report = check_sum_formula(term, claim, 2);
    CHECK(report.base_case);
    CHECK(!report.inductive_step);
}

TEST(3, "a zero denominator throws invalid_argument") {
    CHECK_THROWS_AS(check_sum_formula(poly({1}), poly({0, 1}), 0), std::invalid_argument);
}

TEST(3, "random identities are proved exactly when the constant term is zero") {
    Rng rng;
    for (int step = 0; step < 3'000; ++step) {
        // F(n) - F(0) is the sum of its own backward differences g(i) = F(i) - F(i-1).
        Coefficients f(static_cast<std::size_t>(rng.uniform(1, 5)));
        for (auto& value : f) {
            value = rng.uniform(-20, 20);
        }
        const Coefficients g = backward_difference(f);
        const std::int64_t denominator = rng.uniform(1, 30);
        Coefficients scaled = f;
        for (auto& value : scaled) {
            value *= denominator;
        }
        INFO("step ", step);
        const auto report = check_sum_formula(poly(g), poly(scaled), denominator);
        REQUIRE(report.inductive_step);
        REQUIRE_EQ(report.base_case, f[0] == 0);
    }
}

TEST(3, "random perturbations of a true formula are caught") {
    Rng rng;
    for (int step = 0; step < 2'000; ++step) {
        Coefficients f(static_cast<std::size_t>(rng.uniform(2, 5)));
        for (auto& value : f) {
            value = rng.uniform(-20, 20);
        }
        f[0] = 0;
        const Coefficients g = backward_difference(f);
        Coefficients wrong = f;
        const std::size_t position = static_cast<std::size_t>(
            rng.uniform(1, static_cast<std::int64_t>(wrong.size()) - 1));
        wrong[position] += rng.coin() ? 1 : -1;
        INFO("step ", step);
        const auto report = check_sum_formula(poly(g), poly(wrong), 1);
        REQUIRE(!report.inductive_step);
    }
}
