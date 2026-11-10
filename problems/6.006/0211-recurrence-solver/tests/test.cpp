#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::growth_exponent;

namespace {

struct Case {
    double a;
    double b;
    double c;
    int k;
    MasterCase which;
    double exponent;
    int log_power;
};

// Classic divide-and-conquer shapes and a few synthetic ones.
const std::vector<Case>& cases() {
    const double lg3 = std::log2(3.0);
    const double lg7 = std::log2(7.0);
    static const std::vector<Case> table = {
        {1, 2, 0, 0, MasterCase::Balanced, 0, 1},      // binary search
        {2, 2, 1, 0, MasterCase::Balanced, 1, 1},      // merge sort
        {2, 2, 0, 0, MasterCase::Leaves, 1, 0},        // tree traversal
        {3, 2, 1, 0, MasterCase::Leaves, lg3, 0},      // Karatsuba
        {7, 2, 2, 0, MasterCase::Leaves, lg7, 0},      // Strassen
        {1, 2, 1, 0, MasterCase::Root, 1, 0},          // geometric decrease
        {8, 2, 1, 0, MasterCase::Leaves, 3, 0},        // leaves far outweigh the root
        {4, 2, 3, 0, MasterCase::Root, 3, 0},          // root dominated
        {9, 3, 2, 1, MasterCase::Balanced, 2, 2},      // balanced with an extra log
        {2, 2, 1, 2, MasterCase::Balanced, 1, 3},      // repeated logs
        {2, 4, 0.5, 0, MasterCase::Balanced, 0.5, 1},  // fractional exponents
        {2, 2, 3, 1, MasterCase::Root, 3, 1},          // root dominated with a log
    };
    return table;
}

}  // namespace

TEST(1, "every entry in the table of classic recurrences") {
    for (std::size_t i = 0; i < cases().size(); ++i) {
        const Case& c = cases()[i];
        INFO("case #", i, " a=", c.a, " b=", c.b, " c=", c.c, " k=", c.k);
        const MasterResult result = solve_master(c.a, c.b, c.c, c.k);
        REQUIRE(result.which == c.which);
        CHECK_NEAR(result.growth.exponent, c.exponent, 1e-9);
        CHECK_EQ(result.growth.log_power, c.log_power);
    }
}

TEST(1, "irrational bases are compared with tolerance") {
    // log_b a == 2 exactly for b = sqrt(5), a = 5.
    const MasterResult result = solve_master(5.0, std::sqrt(5.0), 2.0, 0);
    CHECK(result.which == MasterCase::Balanced);
    CHECK_NEAR(result.growth.exponent, 2.0, 1e-9);
    CHECK_EQ(result.growth.log_power, 1);
}

TEST(1, "invalid parameters throw") {
    CHECK_THROWS_AS(solve_master(0.5, 2, 1, 0), std::invalid_argument);
    CHECK_THROWS_AS(solve_master(2, 1, 1, 0), std::invalid_argument);
    CHECK_THROWS_AS(solve_master(2, 0.5, 1, 0), std::invalid_argument);
    CHECK_THROWS_AS(solve_master(2, 2, -1, 0), std::invalid_argument);
    CHECK_THROWS_AS(solve_master(2, 2, 1, -1), std::invalid_argument);
}

TEST(2, "a balanced tree does the same work on every level") {
    const auto levels = level_work(2, 2, 1, 0, 3);  // n = 8
    CHECK_EQ(levels, std::vector<double>{8, 8, 8, 8});
    CHECK_NEAR(tree_total(2, 2, 1, 0, 3), 32, 1e-9);
}

TEST(2, "a leaf-heavy tree grows geometrically toward the bottom") {
    const auto levels = level_work(8, 2, 1, 0, 3);  // n = 8
    CHECK_EQ(levels, std::vector<double>{8, 32, 128, 512});
}

TEST(2, "a root-heavy tree shrinks geometrically") {
    const auto levels = level_work(2, 2, 2, 0, 3);  // n = 8
    CHECK_EQ(levels, std::vector<double>{64, 32, 16, 8});
}

TEST(2, "depth zero is a single leaf") {
    CHECK_EQ(level_work(3, 2, 1, 0, 0), std::vector<double>{1});
    CHECK_NEAR(tree_total(3, 2, 1, 0, 0), 1, 1e-12);
}

TEST(2, "log factors are applied using base 2") {
    // n = 16, a = 1, b = 2, c = 0, k = 1: level sizes 16, 8, 4, 2, leaves.
    const auto levels = level_work(1, 2, 0, 1, 4);
    CHECK_EQ(levels, std::vector<double>{4, 3, 2, 1, 1});
}

TEST(2, "the number of subproblems is a^i, including non-integer branching") {
    const auto levels = level_work(1.5, 2, 0, 0, 2);  // n = 4
    CHECK_NEAR(levels[0], 1.0, 1e-12);
    CHECK_NEAR(levels[1], 1.5, 1e-12);
    CHECK_NEAR(levels[2], 2.25, 1e-12);
}

TEST(2, "the tree total converges to the master theorem's growth") {
    for (std::size_t i = 0; i < cases().size(); ++i) {
        const Case& c = cases()[i];
        const auto result = solve_master(c.a, c.b, c.c, c.k);
        const auto ratio_at = [&](int depth) {
            const double n = std::pow(c.b, depth);
            const double predicted = std::pow(n, result.growth.exponent) *
                                     std::pow(std::log2(n), result.growth.log_power);
            return tree_total(c.a, c.b, c.c, c.k, depth) / predicted;
        };
        INFO("case #", i);
        const double r1 = ratio_at(60);
        const double r2 = ratio_at(120);
        CHECK(std::isfinite(r1) && r1 > 0);
        CHECK_NEAR(r2 / r1, 1.0, 0.1);
    }
}

TEST(3, "O in a leaf-heavy recurrence does not change the answer") {
    const Bounds b = bounds_with_big_o(8, 2, 1, 0);
    CHECK_NEAR(b.upper.exponent, 3, 1e-9);
    CHECK_NEAR(b.lower.exponent, 3, 1e-9);
    CHECK_EQ(b.upper.log_power, 0);
    CHECK_EQ(b.lower.log_power, 0);
    CHECK(b.tight());
}

TEST(3, "O in a root-heavy recurrence leaves a gap") {
    const Bounds b = bounds_with_big_o(2, 2, 4, 0);
    CHECK_NEAR(b.upper.exponent, 4, 1e-9);
    CHECK_NEAR(b.lower.exponent, 1, 1e-9);
    CHECK(!b.tight());
}

TEST(3, "O in a balanced recurrence loses the log in the lower bound") {
    const Bounds b = bounds_with_big_o(2, 2, 1, 0);
    CHECK_NEAR(b.upper.exponent, 1, 1e-9);
    CHECK_EQ(b.upper.log_power, 1);
    CHECK_NEAR(b.lower.exponent, 1, 1e-9);
    CHECK_EQ(b.lower.log_power, 0);
    CHECK(!b.tight());
}

TEST(3, "lower bounds use the critical exponent for non-integer branching") {
    const Bounds b = bounds_with_big_o(3, 2, 5, 0);
    CHECK_NEAR(b.lower.exponent, std::log2(3.0), 1e-9);
    CHECK_EQ(b.lower.log_power, 0);
    CHECK_NEAR(b.upper.exponent, 5, 1e-9);
}

TEST(3, "invalid parameters still throw") {
    CHECK_THROWS_AS(bounds_with_big_o(0, 2, 1, 0), std::invalid_argument);
}

TEST(4, "chain recurrences gain one in the exponent") {
    const Growth g = solve_chain(1, 0);
    CHECK_NEAR(g.exponent, 2, 1e-12);
    CHECK_EQ(g.log_power, 0);
    const Growth h = solve_chain(0, 3);
    CHECK_NEAR(h.exponent, 1, 1e-12);
    CHECK_EQ(h.log_power, 3);
    CHECK_THROWS_AS(solve_chain(-1, 0), std::invalid_argument);
    CHECK_THROWS_AS(solve_chain(1, -1), std::invalid_argument);
}

TEST(4, "chain_work sums the chain exactly") {
    CHECK_EQ(chain_work(1, 1, 5), 15);          // 5 + 4 + 3 + 2 + 1
    CHECK_EQ(chain_work(2, 1, 7), 16);          // 7 + 5 + 3 + 1
    CHECK_EQ(chain_work(3, 2, 9), 81 + 36 + 9); // 9^2 + 6^2 + 3^2
    CHECK_EQ(chain_work(1, 0, 4), 4);
    CHECK_EQ(chain_work(10, 3, 3), 27);         // a single term
}

TEST(4, "doubling n confirms the chain's growth") {
    // T(n) = T(n - s) + n has quadratic growth for any fixed step.
    for (std::uint64_t s : {1ULL, 2ULL, 7ULL}) {
        const auto cost_small = static_cast<double>(chain_work(s, 1, 40'000));
        const auto cost_large = static_cast<double>(chain_work(s, 1, 160'000));
        INFO("s=", s);
        CHECK_NEAR(growth_exponent(cost_small, cost_large, 40'000, 160'000), solve_chain(1, 0).exponent, 0.01);
    }
}
