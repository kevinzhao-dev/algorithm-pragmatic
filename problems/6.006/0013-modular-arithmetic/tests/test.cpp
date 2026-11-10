#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Wide = __int128;

namespace {

constexpr auto kMin = std::numeric_limits<std::int64_t>::min();
constexpr auto kMax = std::numeric_limits<std::int64_t>::max();

std::optional<std::int64_t> brute_coincidence(PeriodicJob a, PeriodicJob b, int horizon) {
    for (std::int64_t t = std::max(a.phase, b.phase); t <= horizon; ++t) {
        const bool a_runs = t >= a.phase && (t - a.phase) % a.period == 0;
        const bool b_runs = t >= b.phase && (t - b.phase) % b.period == 0;
        if (a_runs && b_runs) {
            return t;
        }
    }
    return std::nullopt;
}

// Verifies a claimed answer without enumerating times: both jobs run at t, t is not before
// either job starts, and the previous coincidence (t - lcm) would be too early.
void check_coincidence(PeriodicJob a, PeriodicJob b, std::optional<std::int64_t> answer) {
    const std::int64_t g = std::gcd(a.period, b.period);
    const bool solvable = (b.phase - a.phase) % g == 0;
    REQUIRE_EQ(answer.has_value(), solvable);
    if (!answer) {
        return;
    }
    const std::int64_t t = *answer;
    REQUIRE(t >= a.phase && t >= b.phase);
    CHECK_EQ((t - a.phase) % a.period, 0);
    CHECK_EQ((t - b.phase) % b.period, 0);
    const Wide lcm = static_cast<Wide>(a.period) / g * b.period;
    CHECK(static_cast<Wide>(t) - lcm < std::max(a.phase, b.phase));
}

}  // namespace

TEST(1, "floor_mod of non-negative values matches the remainder") {
    CHECK_EQ(floor_mod(0, 3), 0);
    CHECK_EQ(floor_mod(7, 3), 1);
    CHECK_EQ(floor_mod(9, 3), 0);
    CHECK_EQ(floor_mod(2, 3), 2);
}

TEST(1, "floor_mod of negative values is still in [0, m)") {
    CHECK_EQ(floor_mod(-1, 5), 4);
    CHECK_EQ(floor_mod(-7, 3), 2);
    CHECK_EQ(floor_mod(-6, 3), 0);
    CHECK_EQ(floor_mod(-5, 5), 0);
}

TEST(1, "modulus one maps everything to zero") {
    CHECK_EQ(floor_mod(-7, 1), 0);
    CHECK_EQ(floor_mod(123, 1), 0);
    CHECK_EQ(floor_div(-7, 1), -7);
}

TEST(1, "floor_div rounds toward negative infinity") {
    CHECK_EQ(floor_div(7, 3), 2);
    CHECK_EQ(floor_div(6, 3), 2);
    CHECK_EQ(floor_div(-7, 3), -3);
    CHECK_EQ(floor_div(-6, 3), -2);
    CHECK_EQ(floor_div(-1, 5), -1);
    CHECK_EQ(floor_div(0, 5), 0);
}

TEST(1, "floor_div and floor_mod satisfy the division identity") {
    Rng rng;
    for (int step = 0; step < 50'000; ++step) {
        const auto a = static_cast<std::int64_t>(rng.engine()());
        const std::int64_t m = rng.uniform(1, std::int64_t{1} << 40);
        INFO("a=", a, " m=", m);
        const std::int64_t r = floor_mod(a, m);
        REQUIRE(r >= 0 && r < m);
        REQUIRE_EQ(static_cast<Wide>(m) * floor_div(a, m) + r == static_cast<Wide>(a), true);
    }
}

TEST(1, "extreme values do not overflow") {
    CHECK_EQ(floor_mod(kMin, 3), 1);  // -2^63 = 3 * -3074457345618258603 + 1
    CHECK_EQ(floor_div(kMin, 3), -3'074'457'345'618'258'603LL);
    CHECK_EQ(floor_mod(kMax, kMax), 0);
    CHECK_EQ(floor_mod(kMin, kMax), kMax - 1);
    CHECK_EQ(floor_div(kMin, 1), kMin);
}

TEST(1, "congruent compares residues across zero") {
    CHECK(congruent(-1, 2, 3));
    CHECK(congruent(7, -3, 5));
    CHECK(!congruent(1, 2, 3));
    CHECK(congruent(5, 5, 1'000));
}

TEST(1, "congruent does not overflow at the extremes") {
    const auto expected = [](std::int64_t a, std::int64_t b, std::int64_t m) {
        return (static_cast<Wide>(a) - b) % m == 0;
    };
    CHECK_EQ(congruent(kMax, kMin, 7), expected(kMax, kMin, 7));
    CHECK_EQ(congruent(kMin, kMax, 7), expected(kMin, kMax, 7));
    CHECK_EQ(congruent(kMax, kMin, 3), expected(kMax, kMin, 3));
    CHECK_EQ(congruent(kMax, -1, kMax), expected(kMax, -1, kMax));
}

TEST(1, "a non-positive modulus throws invalid_argument") {
    CHECK_THROWS_AS(floor_mod(5, 0), std::invalid_argument);
    CHECK_THROWS_AS(floor_mod(5, -2), std::invalid_argument);
    CHECK_THROWS_AS(floor_div(5, 0), std::invalid_argument);
    CHECK_THROWS_AS(congruent(1, 2, 0), std::invalid_argument);
}

TEST(2, "mul_mod with values that overflow 64 bits") {
    const std::uint64_t max = std::numeric_limits<std::uint64_t>::max();
    CHECK_EQ(mul_mod(max, max, max - 1), 1);
    CHECK_EQ(mul_mod(3, 5, 7), 1);
    CHECK_EQ(mul_mod(0, 9, 7), 0);
    CHECK_EQ(mul_mod(9, 9, 1), 0);
}

TEST(2, "mul_mod matches 128-bit arithmetic") {
    Rng rng;
    for (int step = 0; step < 100'000; ++step) {
        const std::uint64_t a = rng.engine()();
        const std::uint64_t b = rng.engine()();
        const std::uint64_t m = rng.engine()() | 1U;
        INFO("a=", a, " b=", b, " m=", m);
        const auto expected = static_cast<std::uint64_t>(static_cast<unsigned __int128>(a) * b % m);
        REQUIRE_EQ(mul_mod(a, b, m), expected);
    }
}

TEST(2, "pow_mod edges") {
    CHECK_EQ(pow_mod(0, 0, 7), 1);
    CHECK_EQ(pow_mod(5, 0, 7), 1);
    CHECK_EQ(pow_mod(0, 3, 7), 0);
    CHECK_EQ(pow_mod(9, 1, 7), 2);
    CHECK_EQ(pow_mod(5, 123, 1), 0);
    CHECK_EQ(pow_mod(0, 0, 1), 0);
}

TEST(2, "pow_mod matches repeated multiplication") {
    Rng rng;
    for (int step = 0; step < 3'000; ++step) {
        const auto base = static_cast<std::uint64_t>(rng.uniform(0, 1'000));
        const auto exponent = static_cast<std::uint64_t>(rng.uniform(0, 500));
        const auto m = static_cast<std::uint64_t>(rng.uniform(1, 2'000));
        INFO("base=", base, " exponent=", exponent, " m=", m);
        unsigned __int128 expected = 1 % m;
        for (std::uint64_t i = 0; i < exponent; ++i) {
            expected = expected * base % m;
        }
        REQUIRE_EQ(pow_mod(base, exponent, m), static_cast<std::uint64_t>(expected));
    }
}

TEST(2, "pow_mod agrees with Fermat's little theorem on large primes") {
    const std::uint64_t mersenne61 = (std::uint64_t{1} << 61) - 1;
    Rng rng;
    for (int step = 0; step < 200; ++step) {
        const std::uint64_t a = static_cast<std::uint64_t>(rng.uniform(1, (std::int64_t{1} << 61) - 2));
        INFO("a=", a);
        REQUIRE_EQ(pow_mod(a, mersenne61 - 1, mersenne61), 1);
    }
    CHECK_EQ(pow_mod(2, 1'000'000'006, 1'000'000'007), 1);
}

TEST(2, "exponents add under multiplication") {
    Rng rng;
    for (int step = 0; step < 5'000; ++step) {
        const std::uint64_t a = rng.engine()();
        const auto x = static_cast<std::uint64_t>(rng.uniform(0, 1'000'000));
        const auto y = static_cast<std::uint64_t>(rng.uniform(0, 1'000'000));
        const auto m = static_cast<std::uint64_t>(rng.uniform(1, 2'000'000'000));
        INFO("a=", a, " x=", x, " y=", y, " m=", m);
        REQUIRE_EQ(pow_mod(a, x + y, m), mul_mod(pow_mod(a, x, m), pow_mod(a, y, m), m));
    }
}

// A linear loop cannot finish; the exponents here are around 2^62.
PERF(2, "huge exponents take a logarithmic number of multiplications", 500) {
    std::uint64_t sum = 0;
    const std::uint64_t mersenne61 = (std::uint64_t{1} << 61) - 1;
    for (std::uint64_t base = 2; base < 100'000; ++base) {
        sum += pow_mod(base, (std::uint64_t{1} << 62) + base, mersenne61);
    }
    CHECK_GT(sum, 0);
}

TEST(3, "coprime periods first coincide within one lcm") {
    CHECK_EQ(first_coincidence({1, 4}, {2, 3}), std::optional<std::int64_t>(5));
    CHECK_EQ(first_coincidence({2, 3}, {1, 4}), std::optional<std::int64_t>(5));
}

TEST(3, "identical jobs coincide at their phase") {
    CHECK_EQ(first_coincidence({7, 5}, {7, 5}), std::optional<std::int64_t>(7));
}

TEST(3, "the coincidence is not before either job starts") {
    CHECK_EQ(first_coincidence({0, 2}, {10, 3}), std::optional<std::int64_t>(10));
    CHECK_EQ(first_coincidence({0, 1}, {0, 7}), std::optional<std::int64_t>(0));
}

TEST(3, "jobs that can never coincide") {
    CHECK(!first_coincidence({1, 4}, {2, 6}).has_value());
    CHECK(!first_coincidence({0, 2}, {1, 2}).has_value());
}

TEST(3, "non-coprime periods with a solution") {
    CHECK_EQ(first_coincidence({2, 4}, {0, 6}), std::optional<std::int64_t>(6));
}

TEST(3, "small cases match a brute-force scan") {
    for (std::int64_t pa = 0; pa <= 12; ++pa) {
        for (std::int64_t qa = 1; qa <= 12; ++qa) {
            for (std::int64_t pb = 0; pb <= 12; ++pb) {
                for (std::int64_t qb = 1; qb <= 12; ++qb) {
                    const PeriodicJob a{pa, qa};
                    const PeriodicJob b{pb, qb};
                    INFO("a=(", pa, ",", qa, ") b=(", pb, ",", qb, ")");
                    REQUIRE_EQ(first_coincidence(a, b), brute_coincidence(a, b, 300));
                }
            }
        }
    }
}

TEST(3, "large random jobs satisfy the coincidence properties") {
    Rng rng;
    for (int step = 0; step < 20'000; ++step) {
        const PeriodicJob a{rng.uniform(0, 1'000'000'000'000LL), rng.uniform(1, 1'000'000'000)};
        PeriodicJob b{rng.uniform(0, 1'000'000'000'000LL), rng.uniform(1, 1'000'000'000)};
        if (step % 2 == 0) {
            // Force a shared factor so solvable and unsolvable instances both show up.
            b.period = std::max<std::int64_t>(1, a.period / rng.uniform(1, 40) * rng.uniform(1, 40));
        }
        INFO("a=(", a.phase, ",", a.period, ") b=(", b.phase, ",", b.period, ")");
        check_coincidence(a, b, first_coincidence(a, b));
    }
}

// Enumerating executions of one job would take about 10^9 steps per query.
PERF(3, "periods near 10^9 are solved without enumeration", 200) {
    const PeriodicJob a{123, 999'999'937};
    std::int64_t total = 0;
    for (std::int64_t k = 0; k < 20'000; ++k) {
        const PeriodicJob b{456 + k, 999'999'929 - 2 * k};
        const auto t = first_coincidence(a, b);
        REQUIRE(t.has_value());
        total += *t % 7;
    }
    CHECK_GE(total, 0);
}
