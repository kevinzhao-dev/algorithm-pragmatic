#include <algorithm>
#include <vector>

#include "06_lp_oracle.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

Frac dot(const std::vector<Frac>& a, const std::vector<Frac>& b) {
    Frac out;
    for (std::size_t i = 0; i < a.size(); ++i) {
        out += a[i] * b[i];
    }
    return out;
}

bool feasible(const StandardLp& lp, const std::vector<Frac>& x) {
    for (const Frac& v : x) {
        if (v < Frac(0)) {
            return false;
        }
    }
    for (std::size_t i = 0; i < lp.A.size(); ++i) {
        if (dot(lp.A[i], x) > lp.b[i]) {
            return false;
        }
    }
    return true;
}

// Checks a primal/dual pair against the optimality conditions and the reported value.
void check_certificate(const StandardLp& lp, const LpResult& r) {
    REQUIRE_EQ(r.x.size(), lp.c.size());
    REQUIRE_EQ(r.dual.size(), lp.b.size());
    CHECK(feasible(lp, r.x));
    CHECK_EQ(dot(lp.c, r.x), r.value);
    for (const Frac& y : r.dual) {
        CHECK_GE(y, Frac(0));
    }
    for (std::size_t j = 0; j < lp.c.size(); ++j) {
        Frac lhs;
        for (std::size_t i = 0; i < lp.A.size(); ++i) {
            lhs += lp.A[i][j] * r.dual[i];
        }
        CHECK_GE(lhs, lp.c[j]);
    }
    CHECK_EQ(dot(lp.b, r.dual), r.value);
}

StandardLp random_lp(Rng& rng, std::size_t n, std::size_t m, int low_b) {
    StandardLp lp;
    lp.c.resize(n);
    for (auto& v : lp.c) {
        v = Frac(rng.uniform(-3, 5));
    }
    for (std::size_t i = 0; i < m; ++i) {
        std::vector<Frac> row(n);
        for (auto& v : row) {
            v = Frac(rng.uniform(-3, 4));
        }
        lp.A.push_back(std::move(row));
        lp.b.push_back(Frac(rng.uniform(low_b, 8)));
    }
    return lp;
}

lp_oracle::Status to_oracle(LpStatus s) {
    return s == LpStatus::Optimal
               ? lp_oracle::Status::Optimal
               : (s == LpStatus::Infeasible ? lp_oracle::Status::Infeasible
                                            : lp_oracle::Status::Unbounded);
}

}  // namespace

TEST(1, "a square feasible region") {
    const StandardLp lp{{Frac(1), Frac(1)}, {{Frac(1), Frac(0)}, {Frac(0), Frac(1)}}, {Frac(2), Frac(3)}};
    const std::vector<std::pair<Frac, Frac>> expected{
        {Frac(0), Frac(0)}, {Frac(0), Frac(3)}, {Frac(2), Frac(0)}, {Frac(2), Frac(3)}};
    CHECK_EQ(feasible_vertices_2d(lp), expected);
}

TEST(1, "fractional intersections are exact") {
    const StandardLp lp{{Frac(0), Frac(0)}, {{Frac(2), Frac(1)}, {Frac(1), Frac(3)}}, {Frac(4), Frac(6)}};
    const auto vertices = feasible_vertices_2d(lp);
    REQUIRE_EQ(vertices.size(), 4);
    CHECK_EQ(vertices[1], (std::pair<Frac, Frac>{Frac(0), Frac(2)}));
    CHECK_EQ(vertices[2], (std::pair<Frac, Frac>{Frac(6, 5), Frac(8, 5)}));
    CHECK_EQ(vertices[3], (std::pair<Frac, Frac>{Frac(2), Frac(0)}));
}

TEST(1, "an empty region has no vertices") {
    const StandardLp lp{{Frac(1), Frac(1)}, {{Frac(1), Frac(0)}}, {Frac(-1)}};
    CHECK(feasible_vertices_2d(lp).empty());
}

TEST(1, "redundant constraints do not duplicate vertices") {
    const StandardLp lp{
        {Frac(1), Frac(1)},
        {{Frac(1), Frac(1)}, {Frac(2), Frac(2)}, {Frac(1), Frac(0)}},
        {Frac(4), Frac(8), Frac(4)}};
    const auto vertices = feasible_vertices_2d(lp);
    CHECK_EQ(vertices.size(), 3);
    CHECK(std::is_sorted(vertices.begin(), vertices.end()));
    CHECK(std::adjacent_find(vertices.begin(), vertices.end()) == vertices.end());
}

TEST(1, "the optimum of a two-variable LP is at a vertex") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        auto lp = random_lp(rng, 2, static_cast<std::size_t>(rng.uniform(1, 4)), 0);
        const auto vertices = feasible_vertices_2d(lp);
        INFO("round ", round);
        Frac best = Frac(-1'000'000);
        bool any = false;
        for (const auto& [x, y] : vertices) {
            REQUIRE(feasible(lp, {x, y}));
            const Frac value = lp.c[0] * x + lp.c[1] * y;
            if (!any || value > best) {
                best = value;
                any = true;
            }
        }
        const auto oracle = lp_oracle::maximize(lp.A, lp.b, lp.c);
        CHECK_EQ(any, oracle.status != lp_oracle::Status::Infeasible);
        if (oracle.status == lp_oracle::Status::Optimal) {
            CHECK_EQ(best, oracle.value);
        }
    }
}

TEST(2, "a small problem is solved exactly") {
    // maximize 7x + 5y  s.t.  x + y <= 6, 2x + y <= 9
    const StandardLp lp{{Frac(7), Frac(5)}, {{Frac(1), Frac(1)}, {Frac(2), Frac(1)}}, {Frac(6), Frac(9)}};
    const auto r = solve_lp(lp);
    REQUIRE(r.status == LpStatus::Optimal);
    CHECK_EQ(r.value, Frac(36));
    CHECK_EQ(r.x, (std::vector<Frac>{Frac(3), Frac(3)}));
}

TEST(2, "unbounded problems are detected") {
    const StandardLp lp{{Frac(1)}, {{Frac(-1)}}, {Frac(3)}};
    CHECK(solve_lp(lp).status == LpStatus::Unbounded);
}

TEST(2, "a zero objective stops immediately at the origin") {
    const StandardLp lp{{Frac(0), Frac(0)}, {{Frac(1), Frac(1)}}, {Frac(5)}};
    const auto r = solve_lp(lp);
    REQUIRE(r.status == LpStatus::Optimal);
    CHECK_EQ(r.value, Frac(0));
}

TEST(2, "the trace walks through improving feasible vertices") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 4));
        const auto m = static_cast<std::size_t>(rng.uniform(1, 4));
        const auto lp = random_lp(rng, n, m, 0);
        std::vector<std::vector<Frac>> trace;
        const auto r = solve_lp(lp, &trace);
        INFO("round ", round);
        REQUIRE(!trace.empty());
        CHECK_EQ(trace.front(), std::vector<Frac>(n));
        for (const auto& point : trace) {
            REQUIRE_EQ(point.size(), n);
            CHECK(feasible(lp, point));
        }
        for (std::size_t i = 1; i < trace.size(); ++i) {
            CHECK_GE(dot(lp.c, trace[i]), dot(lp.c, trace[i - 1]));
        }
    }
}

TEST(2, "bland's rule survives a classic cycling instance") {
    // Beale's degenerate example; a largest-coefficient rule cycles forever on it.
    const StandardLp lp{
        {Frac(3, 4), Frac(-20), Frac(1, 2), Frac(-6)},
        {
            {Frac(1, 4), Frac(-8), Frac(-1), Frac(9)},
            {Frac(1, 2), Frac(-12), Frac(-1, 2), Frac(3)},
            {Frac(0), Frac(0), Frac(1), Frac(0)},
        },
        {Frac(0), Frac(0), Frac(1)}};
    const auto r = solve_lp(lp);
    REQUIRE(r.status == LpStatus::Optimal);
    CHECK_EQ(r.value, Frac(5, 4));
    CHECK(feasible(lp, r.x));
}

TEST(2, "random origin-feasible LPs match vertex enumeration") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 4));
        const auto m = static_cast<std::size_t>(rng.uniform(1, 4));
        const auto lp = random_lp(rng, n, m, 0);
        const auto r = solve_lp(lp);
        const auto oracle = lp_oracle::maximize(lp.A, lp.b, lp.c);
        INFO("round ", round);
        REQUIRE(to_oracle(r.status) == oracle.status);
        if (r.status == LpStatus::Optimal) {
            CHECK_EQ(r.value, oracle.value);
            CHECK(feasible(lp, r.x));
            CHECK_EQ(dot(lp.c, r.x), r.value);
        }
    }
}

TEST(3, "negative right-hand side, still feasible") {
    // x + y >= 2  written as -x - y <= -2;  maximize -x - y
    const StandardLp lp{{Frac(-1), Frac(-1)}, {{Frac(-1), Frac(-1)}}, {Frac(-2)}};
    const auto r = solve_lp(lp);
    REQUIRE(r.status == LpStatus::Optimal);
    CHECK_EQ(r.value, Frac(-2));
    CHECK(feasible(lp, r.x));
}

TEST(3, "contradictory constraints are infeasible") {
    const StandardLp lp{{Frac(1)}, {{Frac(1)}, {Frac(-1)}}, {Frac(1), Frac(-3)}};
    CHECK(solve_lp(lp).status == LpStatus::Infeasible);
}

TEST(3, "unbounded after phase I") {
    const StandardLp lp{{Frac(1), Frac(0)}, {{Frac(-1), Frac(1)}}, {Frac(-2)}};
    CHECK(solve_lp(lp).status == LpStatus::Unbounded);
}

TEST(3, "random LPs with negative b match vertex enumeration") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 3));
        const auto m = static_cast<std::size_t>(rng.uniform(1, 4));
        const auto lp = random_lp(rng, n, m, -5);
        std::vector<std::vector<Frac>> trace;
        const auto r = solve_lp(lp, &trace);
        const auto oracle = lp_oracle::maximize(lp.A, lp.b, lp.c);
        INFO("round ", round);
        REQUIRE(to_oracle(r.status) == oracle.status);
        if (r.status == LpStatus::Optimal) {
            CHECK_EQ(r.value, oracle.value);
            CHECK(feasible(lp, r.x));
        }
        for (std::size_t i = 0; i < trace.size(); ++i) {
            CHECK(feasible(lp, trace[i]));
        }
    }
}

TEST(4, "the dual certifies a small problem") {
    const StandardLp lp{{Frac(7), Frac(5)}, {{Frac(1), Frac(1)}, {Frac(2), Frac(1)}}, {Frac(6), Frac(9)}};
    const auto r = solve_lp(lp);
    REQUIRE(r.status == LpStatus::Optimal);
    check_certificate(lp, r);
}

TEST(4, "dual certificates hold on random LPs") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 5));
        const auto m = static_cast<std::size_t>(rng.uniform(1, 5));
        const auto lp = random_lp(rng, n, m, -4);
        const auto r = solve_lp(lp);
        INFO("round ", round);
        if (r.status == LpStatus::Optimal) {
            check_certificate(lp, r);
        }
    }
}

TEST(4, "exact arithmetic survives a mid-sized dense LP") {
    Rng rng;
    const std::size_t n = 10, m = 10;
    StandardLp lp;
    lp.c.resize(n);
    for (auto& v : lp.c) {
        v = Frac(rng.uniform(1, 9));
    }
    for (std::size_t i = 0; i < m; ++i) {
        std::vector<Frac> row(n);
        for (auto& v : row) {
            v = Frac(rng.uniform(0, 9));
        }
        lp.A.push_back(std::move(row));
        lp.b.push_back(Frac(rng.uniform(5, 50)));
    }
    const auto r = solve_lp(lp);
    REQUIRE(r.status == LpStatus::Optimal);
    check_certificate(lp, r);
}
