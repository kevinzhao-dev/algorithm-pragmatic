#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Wide = __int128;

namespace {

constexpr std::int64_t kMax = std::numeric_limits<std::int64_t>::max();

Wide wide_gcd(Wide a, Wide b) {
    while (b != 0) {
        const Wide next = a % b;
        a = b;
        b = next;
    }
    return a < 0 ? -a : a;
}

// Reduces n/d with 128-bit arithmetic and compares it with `f`.
bool matches(const Fraction& f, Wide n, Wide d) {
    if (d < 0) {
        n = -n;
        d = -d;
    }
    const Wide g = wide_gcd(n, d);
    n /= g;
    d /= g;
    return f.numerator() == n && f.denominator() == d;
}

// Interprets a table of probability numerators over a common denominator as a Pmf.
Pmf<int> from_table(const std::vector<std::pair<int, std::int64_t>>& weights,
                    std::int64_t denominator) {
    std::vector<std::pair<int, Fraction>> outcomes;
    for (const auto& [value, weight] : weights) {
        outcomes.emplace_back(value, Fraction(weight, denominator));
    }
    return Pmf<int>(outcomes);
}

Pmf<int> die(int sides) {
    std::vector<int> faces(static_cast<std::size_t>(sides));
    std::iota(faces.begin(), faces.end(), 1);
    return Pmf<int>::uniform(faces);
}

}  // namespace

TEST(1, "the constructor reduces and normalizes the sign") {
    Fraction half(2, 4);
    CHECK_EQ(half.numerator(), 1);
    CHECK_EQ(half.denominator(), 2);

    Fraction both_negative(-6, -8);
    CHECK_EQ(both_negative.numerator(), 3);
    CHECK_EQ(both_negative.denominator(), 4);

    Fraction negative_denominator(3, -9);
    CHECK_EQ(negative_denominator.numerator(), -1);
    CHECK_EQ(negative_denominator.denominator(), 3);

    Fraction whole(7);
    CHECK_EQ(whole.numerator(), 7);
    CHECK_EQ(whole.denominator(), 1);
}

TEST(1, "zero is always 0/1") {
    Fraction empty;
    CHECK_EQ(empty.numerator(), 0);
    CHECK_EQ(empty.denominator(), 1);
    Fraction negative_zero(0, -5);
    CHECK_EQ(negative_zero.numerator(), 0);
    CHECK_EQ(negative_zero.denominator(), 1);
}

TEST(1, "a zero denominator throws invalid_argument") {
    CHECK_THROWS_AS(Fraction(1, 0), std::invalid_argument);
    CHECK_THROWS_AS(Fraction(0, 0), std::invalid_argument);
}

TEST(1, "arithmetic on simple fractions") {
    CHECK_EQ(Fraction(1, 2) + Fraction(1, 3), Fraction(5, 6));
    CHECK_EQ(Fraction(1, 2) - Fraction(1, 3), Fraction(1, 6));
    CHECK_EQ(Fraction(2, 3) * Fraction(9, 4), Fraction(3, 2));
    CHECK_EQ(Fraction(2, 3) / Fraction(4, 5), Fraction(5, 6));
    CHECK_EQ(Fraction(1, 2) - Fraction(1, 2), Fraction(0));
    CHECK_EQ(Fraction(-1, 2) * Fraction(-2, 3), Fraction(1, 3));
}

TEST(1, "results stay reduced") {
    const Fraction sum = Fraction(1, 6) + Fraction(1, 3);
    CHECK_EQ(sum.numerator(), 1);
    CHECK_EQ(sum.denominator(), 2);
    const Fraction product = Fraction(4, 9) * Fraction(3, 8);
    CHECK_EQ(product.numerator(), 1);
    CHECK_EQ(product.denominator(), 6);
}

TEST(1, "division by zero throws domain_error") {
    CHECK_THROWS_AS(Fraction(1, 2) / Fraction(0), std::domain_error);
    CHECK_THROWS_AS(Fraction(0) / Fraction(0, 7), std::domain_error);
}

TEST(1, "comparison is exact when cross products need more than 64 bits") {
    const Fraction a(kMax - 1, 3);
    const Fraction b(kMax, 3);
    CHECK(a < b);
    CHECK(b > a);
    CHECK(a == Fraction(kMax - 1, 3));
    CHECK(a != b);
    CHECK(Fraction(-1, 2) < Fraction(1, 3));
    CHECK(Fraction(1, 3) <= Fraction(2, 6));
    CHECK(Fraction(kMax, kMax - 1) < Fraction(kMax - 1, kMax - 2));
}

TEST(1, "intermediate values may exceed 64 bits when the reduced result fits") {
    const Fraction big(std::int64_t{1} << 62, 3);
    CHECK_EQ(big * Fraction(3, std::int64_t{1} << 61), Fraction(2));
    CHECK_EQ(big - big, Fraction(0));
    CHECK_EQ(big + Fraction(1, 3), Fraction((std::int64_t{1} << 62) + 1, 3));
    CHECK_EQ(Fraction(1, std::int64_t{1} << 62) + Fraction(1, std::int64_t{1} << 62),
             Fraction(1, std::int64_t{1} << 61));
}

TEST(1, "a result that cannot fit throws overflow_error") {
    CHECK_THROWS_AS(Fraction(kMax) + Fraction(1), std::overflow_error);
    const Fraction small(1, 3'037'000'500);
    CHECK_THROWS_AS(small * small, std::overflow_error);
    CHECK_THROWS_AS(Fraction(kMax) * Fraction(3), std::overflow_error);
}

TEST(1, "random arithmetic matches exact 128-bit computation") {
    Rng rng;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const std::int64_t a = rng.uniform(-1'000'000, 1'000'000);
        const std::int64_t b = rng.uniform(1, 1'000'000);
        const std::int64_t c = rng.uniform(-1'000'000, 1'000'000);
        const std::int64_t d = rng.uniform(1, 1'000'000);
        const Fraction x(a, b);
        const Fraction y(c, d);
        REQUIRE(matches(x + y, Wide{a} * d + Wide{c} * b, Wide{b} * d));
        REQUIRE(matches(x - y, Wide{a} * d - Wide{c} * b, Wide{b} * d));
        REQUIRE(matches(x * y, Wide{a} * c, Wide{b} * d));
        if (c != 0) {
            REQUIRE(matches(x / y, Wide{a} * d, Wide{b} * c));
        }
        REQUIRE_EQ(x < y, Wide{a} * d < Wide{c} * b);
        REQUIRE_EQ(x == y, Wide{a} * d == Wide{c} * b);
    }
}

TEST(2, "a uniform die has the expected probabilities and mean") {
    const Pmf<int> d4 = Pmf<int>::uniform({1, 2, 3, 4});
    CHECK_EQ(d4.support(), std::vector<int>{1, 2, 3, 4});
    CHECK_EQ(d4.prob(3), Fraction(1, 4));
    CHECK_EQ(d4.expected(), Fraction(5, 2));
}

TEST(2, "prob of an outcome outside the support is zero") {
    const Pmf<int> coin = Pmf<int>::uniform({0, 1});
    CHECK_EQ(coin.prob(-1), Fraction(0));
    CHECK_EQ(coin.prob(2), Fraction(0));
}

TEST(2, "the weights of equal outcomes add up") {
    const Pmf<int> d = Pmf<int>({{1, Fraction(1, 6)}, {2, Fraction(1, 2)}, {1, Fraction(1, 3)}});
    CHECK_EQ(d.support(), std::vector<int>{1, 2});
    CHECK_EQ(d.prob(1), Fraction(1, 2));
    const Pmf<int> repeated = Pmf<int>::uniform({1, 1, 2, 3});
    CHECK_EQ(repeated.prob(1), Fraction(1, 2));
    CHECK_EQ(repeated.prob(2), Fraction(1, 4));
}

TEST(2, "invalid probabilities throw invalid_argument") {
    using Pairs = std::vector<std::pair<int, Fraction>>;
    CHECK_THROWS_AS(Pmf<int>(Pairs{{1, Fraction(1, 2)}}), std::invalid_argument);
    CHECK_THROWS_AS(Pmf<int>(Pairs{{1, Fraction(3, 2)}, {2, Fraction(-1, 2)}}),
                    std::invalid_argument);
    CHECK_THROWS_AS(Pmf<int>(Pairs{}), std::invalid_argument);
}

TEST(2, "zero-probability outcomes are not in the support") {
    const Pmf<int> d = Pmf<int>({{1, Fraction(1)}, {2, Fraction(0)}});
    CHECK_EQ(d.support(), std::vector<int>{1});
    CHECK_EQ(d.prob(2), Fraction(0));
    CHECK_EQ(d.expected(), Fraction(1));
}

TEST(2, "expected value of a weighted distribution") {
    const Pmf<int> d = Pmf<int>({{0, Fraction(1, 2)}, {1, Fraction(1, 3)}, {6, Fraction(1, 6)}});
    CHECK_EQ(d.expected(), Fraction(0 + 2, 6) + Fraction(6, 6));
}

TEST(2, "expected_of applies a function to each outcome") {
    const Pmf<int> d = die(6);
    CHECK_EQ(d.expected_of([](int x) { return x * x; }), Fraction(91, 6));
    CHECK_EQ(d.expected_of([](int x) { return Fraction(x, 2); }), Fraction(7, 4));
}

TEST(2, "map pushes probability through a function and merges collisions") {
    const Pmf<int> d = Pmf<int>::uniform({-2, -1, 1, 2});
    const auto square = d.map([](int x) { return x * x; });
    CHECK_EQ(square.support(), std::vector<int>{1, 4});
    CHECK_EQ(square.prob(1), Fraction(1, 2));
    CHECK_EQ(square.prob(4), Fraction(1, 2));
}

TEST(2, "random distributions match direct computation") {
    Rng rng;
    for (int round = 0; round < 2'000; ++round) {
        INFO("round ", round);
        const std::int64_t denominator = 360'360;
        std::vector<std::pair<int, std::int64_t>> table;
        std::int64_t remaining = denominator;
        Wide mean_numerator = 0;
        Wide squares_numerator = 0;
        int outcome = static_cast<int>(rng.uniform(-50, 50));
        while (remaining > 0) {
            const std::int64_t weight =
                table.size() == 7 ? remaining : rng.uniform(1, remaining);
            table.emplace_back(outcome, weight);
            mean_numerator += Wide{outcome} * weight;
            squares_numerator += Wide{outcome} * outcome * weight;
            remaining -= weight;
            outcome += static_cast<int>(rng.uniform(1, 3));
        }
        const Pmf<int> d = from_table(table, denominator);
        REQUIRE(matches(d.expected(), mean_numerator, denominator));
        REQUIRE(matches(d.expected_of([](int x) { return x * x; }), squares_numerator, denominator));
    }
}

TEST(3, "independent pairs multiply probabilities") {
    const Pmf<int> coin = Pmf<int>({{0, Fraction(1, 3)}, {1, Fraction(2, 3)}});
    const Pmf<char> letter = Pmf<char>::uniform({'a', 'b'});
    const auto joint = independent(coin, letter);
    using Pair = std::pair<int, char>;
    CHECK_EQ(joint.prob(Pair{1, 'a'}), Fraction(1, 3));
    CHECK_EQ(joint.prob(Pair{0, 'b'}), Fraction(1, 6));
    CHECK_EQ(joint.prob(Pair{2, 'a'}), Fraction(0));
    CHECK_EQ(joint.support().size(), 4);
}

TEST(3, "E[X+Y] = E[X]+E[Y] for independent variables") {
    const Pmf<int> a = die(4);
    const Pmf<int> b = Pmf<int>({{0, Fraction(1, 4)}, {3, Fraction(3, 4)}});
    const auto joint = independent(a, b);
    const Fraction sum = joint.expected_of([](const auto& p) { return p.first + p.second; });
    CHECK_EQ(sum, a.expected() + b.expected());
}

TEST(3, "E[X+Y] = E[X]+E[Y] even when the variables are dependent") {
    // Y is determined by X; the marginals are not independent at all.
    const Pmf<int> x = Pmf<int>({{0, Fraction(1, 2)}, {1, Fraction(1, 2)}});
    using Pair = std::pair<int, int>;
    const Pmf<Pair> joint({{Pair{0, 3}, Fraction(1, 2)}, {Pair{1, 5}, Fraction(1, 2)}});
    const Fraction e_x = joint.expected_of([](const Pair& p) { return p.first; });
    const Fraction e_y = joint.expected_of([](const Pair& p) { return p.second; });
    const Fraction e_sum = joint.expected_of([](const Pair& p) { return p.first + p.second; });
    CHECK_EQ(e_x, x.expected());
    CHECK_EQ(e_sum, e_x + e_y);
}

TEST(3, "E[XY] matches E[X]E[Y] under independence but not under dependence") {
    const Pmf<int> x = die(4);
    const Pmf<int> y = die(8);
    const auto joint = independent(x, y);
    CHECK_EQ(joint.expected_of([](const auto& p) { return p.first * p.second; }),
             x.expected() * y.expected());

    using Pair = std::pair<int, int>;
    const Pmf<Pair> same({{Pair{0, 0}, Fraction(1, 2)}, {Pair{1, 1}, Fraction(1, 2)}});
    const Fraction e_x = same.expected_of([](const Pair& p) { return p.first; });
    const Fraction e_xy = same.expected_of([](const Pair& p) { return p.first * p.second; });
    CHECK_EQ(e_xy, Fraction(1, 2));
    CHECK_NE(e_xy, e_x * e_x);
}

TEST(4, "convolve adds independent variables") {
    const auto two_d4 = convolve(die(4), die(4));
    CHECK_EQ(two_d4.support(), std::vector<int>{2, 3, 4, 5, 6, 7, 8});
    CHECK_EQ(two_d4.prob(2), Fraction(1, 16));
    CHECK_EQ(two_d4.prob(5), Fraction(4, 16));
    CHECK_EQ(two_d4.prob(8), Fraction(1, 16));
    CHECK_EQ(two_d4.expected(), Fraction(5));
}

TEST(4, "heads over many flips by repeated convolution") {
    const Pmf<int> flip = Pmf<int>::uniform({0, 1});
    Pmf<int> heads = Pmf<int>({{0, Fraction(1)}});
    for (int i = 0; i < 20; ++i) {
        heads = convolve(heads, flip);
    }
    CHECK_EQ(heads.expected(), Fraction(10));
    CHECK_EQ(heads.prob(0), Fraction(1, 1 << 20));
    CHECK_EQ(heads.prob(20), Fraction(1, 1 << 20));
}

TEST(4, "at_least_k_up edges") {
    CHECK_EQ(at_least_k_up(3, Fraction(1, 2), 0), Fraction(1));
    CHECK_EQ(at_least_k_up(3, Fraction(1, 2), -2), Fraction(1));
    CHECK_EQ(at_least_k_up(3, Fraction(1, 2), 4), Fraction(0));
    CHECK_EQ(at_least_k_up(5, Fraction(1), 5), Fraction(1));
    CHECK_EQ(at_least_k_up(5, Fraction(0), 1), Fraction(0));
}

TEST(4, "a majority quorum of three replicas") {
    // 0.99^3 + 3 * 0.99^2 * 0.01 = 0.999702
    CHECK_EQ(at_least_k_up(3, Fraction(99, 100), 2), Fraction(999'702, 1'000'000));
}

TEST(4, "at_least_k_up matches the binomial formula") {
    const Fraction p(3, 4);
    const Fraction q(1, 4);
    for (int n = 1; n <= 14; ++n) {
        for (int k = 0; k <= n; ++k) {
            INFO("n=", n, " k=", k);
            Fraction expected;
            for (int j = k; j <= n; ++j) {
                Fraction binom(1);
                for (int i = 1; i <= j; ++i) {
                    binom = binom * Fraction(n - j + i, i);
                }
                Fraction term = binom;
                for (int i = 0; i < j; ++i) {
                    term = term * p;
                }
                for (int i = 0; i < n - j; ++i) {
                    term = term * q;
                }
                expected = expected + term;
            }
            REQUIRE_EQ(at_least_k_up(n, p, k), expected);
        }
    }
}
