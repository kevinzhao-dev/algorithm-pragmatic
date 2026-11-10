#include <algorithm>
#include <functional>
#include <limits>
#include <vector>

#include "06_lp_oracle.hpp"
#include "practice.hpp"
#include "solution.hpp"

using lp_oracle::Status;
using practice::Rng;

namespace {

std::vector<Frac> random_row(Rng& rng, std::size_t n) {
    std::vector<Frac> row(n);
    for (auto& v : row) {
        v = Frac(rng.uniform(-3, 4));
    }
    return row;
}

Frac dot(const std::vector<Frac>& a, std::span<const Frac> b) {
    Frac out;
    for (std::size_t i = 0; i < a.size(); ++i) {
        out += a[i] * b[i];
    }
    return out;
}

// Direct evaluation of a general LP: the semantic meaning of feasibility, independent of any
// conversion code.
bool general_feasible(const GeneralLp& lp, std::span<const Frac> x) {
    for (std::size_t j = 0; j < lp.objective.size(); ++j) {
        if (lp.nonnegative[j] && x[j] < Frac(0)) {
            return false;
        }
    }
    for (const auto& c : lp.constraints) {
        const Frac lhs = dot(c.coeffs, x);
        if (c.relation == Relation::LessEq && lhs > c.rhs) {
            return false;
        }
        if (c.relation == Relation::GreaterEq && lhs < c.rhs) {
            return false;
        }
        if (c.relation == Relation::Equal && lhs != c.rhs) {
            return false;
        }
    }
    return true;
}

// A reference standard-form conversion used only as an oracle for the expected optimum.
StandardLp reference_standard(const GeneralLp& lp) {
    StandardLp out;
    const std::size_t n = lp.objective.size();
    std::vector<int> pos(n), neg(n, -1);
    int columns = 0;
    for (std::size_t j = 0; j < n; ++j) {
        pos[j] = columns++;
        if (!lp.nonnegative[j]) {
            neg[j] = columns++;
        }
    }
    const auto widen = [&](const std::vector<Frac>& v, Frac sign) {
        std::vector<Frac> row(columns);
        for (std::size_t j = 0; j < n; ++j) {
            row[pos[j]] = sign * v[j];
            if (neg[j] != -1) {
                row[neg[j]] = -(sign * v[j]);
            }
        }
        return row;
    };
    out.c = widen(lp.objective, lp.maximize ? Frac(1) : Frac(-1));
    const auto add = [&](const std::vector<Frac>& coeffs, Frac rhs, Frac sign) {
        out.A.push_back(widen(coeffs, sign));
        out.b.push_back(sign * rhs);
    };
    for (const auto& c : lp.constraints) {
        if (c.relation != Relation::GreaterEq) {
            add(c.coeffs, c.rhs, Frac(1));
        }
        if (c.relation != Relation::LessEq) {
            add(c.coeffs, c.rhs, Frac(-1));
        }
    }
    return out;
}

lp_oracle::Answer solve_standard(const StandardLp& lp) {
    return lp_oracle::maximize(lp.A, lp.b, lp.c);
}

GeneralLp random_general(Rng& rng, std::size_t n, std::size_t m) {
    GeneralLp lp;
    lp.maximize = rng.coin();
    lp.objective = random_row(rng, n);
    for (std::size_t j = 0; j < n; ++j) {
        lp.nonnegative.push_back(rng.coin(0.6));
    }
    for (std::size_t i = 0; i < m; ++i) {
        Relation relation = Relation::LessEq;
        const auto r = rng.uniform(0, 2);
        relation = r == 0 ? Relation::LessEq : (r == 1 ? Relation::GreaterEq : Relation::Equal);
        lp.constraints.push_back({random_row(rng, n), relation, Frac(rng.uniform(-5, 8))});
    }
    return lp;
}

}  // namespace

TEST(1, "an already-standard LP keeps its optimum") {
    GeneralLp lp;
    lp.maximize = true;
    lp.objective = {Frac(1), Frac(2)};
    lp.nonnegative = {true, true};
    lp.constraints.push_back({{Frac(1), Frac(1)}, Relation::LessEq, Frac(4)});
    lp.constraints.push_back({{Frac(0), Frac(1)}, Relation::LessEq, Frac(3)});
    const auto form = to_standard(lp);
    REQUIRE_EQ(form.lp.c.size(), 2);
    const auto answer = solve_standard(form.lp);
    REQUIRE(answer.status == Status::Optimal);
    CHECK_EQ(recover_value(form, answer.value), Frac(7));
    const auto x = recover_solution(form, answer.x);
    CHECK(general_feasible(lp, x));
}

TEST(1, "minimization negates the objective") {
    GeneralLp lp;
    lp.maximize = false;
    lp.objective = {Frac(1), Frac(1)};
    lp.nonnegative = {true, true};
    lp.constraints.push_back({{Frac(1), Frac(2)}, Relation::GreaterEq, Frac(4)});
    const auto form = to_standard(lp);
    const auto answer = solve_standard(form.lp);
    REQUIRE(answer.status == Status::Optimal);
    CHECK_EQ(recover_value(form, answer.value), Frac(2));
    CHECK(general_feasible(lp, recover_solution(form, answer.x)));
}

TEST(1, "free variables can go negative") {
    GeneralLp lp;
    lp.maximize = true;
    lp.objective = {Frac(-1)};
    lp.nonnegative = {false};
    lp.constraints.push_back({{Frac(1)}, Relation::GreaterEq, Frac(-5)});
    const auto form = to_standard(lp);
    const auto answer = solve_standard(form.lp);
    REQUIRE(answer.status == Status::Optimal);
    CHECK_EQ(recover_value(form, answer.value), Frac(5));
    CHECK_EQ(recover_solution(form, answer.x), std::vector<Frac>{Frac(-5)});
}

TEST(1, "equalities are enforced in both directions") {
    GeneralLp lp;
    lp.maximize = true;
    lp.objective = {Frac(1), Frac(1)};
    lp.nonnegative = {true, true};
    lp.constraints.push_back({{Frac(1), Frac(-1)}, Relation::Equal, Frac(2)});
    lp.constraints.push_back({{Frac(1), Frac(0)}, Relation::LessEq, Frac(5)});
    const auto form = to_standard(lp);
    const auto answer = solve_standard(form.lp);
    REQUIRE(answer.status == Status::Optimal);
    CHECK_EQ(recover_value(form, answer.value), Frac(8));
    CHECK_EQ(recover_solution(form, answer.x), (std::vector<Frac>{Frac(5), Frac(3)}));
}

TEST(1, "infeasible general problems stay infeasible") {
    GeneralLp lp;
    lp.maximize = true;
    lp.objective = {Frac(1)};
    lp.nonnegative = {false};
    lp.constraints.push_back({{Frac(1)}, Relation::Equal, Frac(1)});
    lp.constraints.push_back({{Frac(1)}, Relation::Equal, Frac(2)});
    const auto form = to_standard(lp);
    CHECK(solve_standard(form.lp).status == Status::Infeasible);
}

TEST(1, "random general LPs match a reference conversion") {
    Rng rng;
    for (int round = 0; round < 150; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 2));
        const auto m = static_cast<std::size_t>(rng.uniform(1, 3));
        const GeneralLp lp = random_general(rng, n, m);
        INFO("round ", round);
        const auto form = to_standard(lp);
        const auto got = solve_standard(form.lp);
        const auto expected = solve_standard(reference_standard(lp));
        REQUIRE(got.status == expected.status);
        if (got.status == Status::Optimal) {
            const Frac expected_value = lp.maximize ? expected.value : -expected.value;
            CHECK_EQ(recover_value(form, got.value), expected_value);
            const auto x = recover_solution(form, got.x);
            REQUIRE_EQ(x.size(), n);
            CHECK(general_feasible(lp, x));
            CHECK_EQ(dot(lp.objective, x), expected_value);
        }
    }
}

namespace {

Frac value_of(const SlackForm& form, const std::vector<Frac>& assignment) {
    Frac z = form.v;
    for (std::size_t j = 0; j < form.nonbasic.size(); ++j) {
        z += form.c[j] * assignment[static_cast<std::size_t>(form.nonbasic[j])];
    }
    return z;
}

std::vector<Frac> complete_assignment(const SlackForm& form, std::vector<Frac> assignment) {
    for (std::size_t i = 0; i < form.basic.size(); ++i) {
        Frac lhs = form.b[i];
        for (std::size_t j = 0; j < form.nonbasic.size(); ++j) {
            lhs -= form.a[i][j] * assignment[static_cast<std::size_t>(form.nonbasic[j])];
        }
        assignment[static_cast<std::size_t>(form.basic[i])] = lhs;
    }
    return assignment;
}

}  // namespace

TEST(2, "to_slack puts original variables as nonbasic") {
    const StandardLp lp{{Frac(2), Frac(3)}, {{Frac(1), Frac(1)}, {Frac(2), Frac(-1)}}, {Frac(4), Frac(5)}};
    const auto form = to_slack(lp);
    CHECK_EQ(form.nonbasic, std::vector<int>{0, 1});
    CHECK_EQ(form.basic, std::vector<int>{2, 3});
    CHECK_EQ(form.b, std::vector<Frac>{Frac(4), Frac(5)});
    CHECK_EQ(form.c, std::vector<Frac>{Frac(2), Frac(3)});
    CHECK_EQ(form.v, Frac(0));
}

TEST(2, "a worked pivot by hand") {
    // x3 = 8 - x1 - 2x2;  z = 3x1 + x2.   Pivot x1 (e=0) in, x3 (l=0) out.
    SlackForm form;
    form.nonbasic = {0, 1};
    form.basic = {2};
    form.a = {{Frac(1), Frac(2)}};
    form.b = {Frac(8)};
    form.c = {Frac(3), Frac(1)};
    form.v = Frac(0);
    const SlackForm out = pivot(form, 0, 0);
    CHECK_EQ(out.nonbasic, std::vector<int>{2, 1});
    CHECK_EQ(out.basic, std::vector<int>{0});
    // x1 = 8 - x3 - 2x2;  z = 24 - 3x3 - 5x2.
    CHECK_EQ(out.b, std::vector<Frac>{Frac(8)});
    CHECK_EQ(out.a[0], (std::vector<Frac>{Frac(1), Frac(2)}));
    CHECK_EQ(out.v, Frac(24));
    CHECK_EQ(out.c, (std::vector<Frac>{Frac(-3), Frac(-5)}));
}

TEST(2, "a fractional pivot reduces correctly") {
    SlackForm form;
    form.nonbasic = {0};
    form.basic = {1};
    form.a = {{Frac(3)}};
    form.b = {Frac(2)};
    form.c = {Frac(4)};
    const SlackForm out = pivot(form, 0, 0);
    CHECK_EQ(out.a[0][0], Frac(1, 3));
    CHECK_EQ(out.b[0], Frac(2, 3));
    CHECK_EQ(out.c[0], Frac(-4, 3));
    CHECK_EQ(out.v, Frac(8, 3));
}

TEST(2, "pivots preserve the solution set and objective") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 4));
        const auto m = static_cast<std::size_t>(rng.uniform(1, 4));
        StandardLp lp;
        lp.c = random_row(rng, n);
        for (std::size_t i = 0; i < m; ++i) {
            lp.A.push_back(random_row(rng, n));
            lp.b.push_back(Frac(rng.uniform(0, 8)));
        }
        SlackForm form = to_slack(lp);
        // Random sequence of pivots with nonzero pivot elements.
        for (int steps = 0; steps < 5; ++steps) {
            std::vector<std::pair<int, int>> choices;
            for (int i = 0; i < static_cast<int>(m); ++i) {
                for (int j = 0; j < static_cast<int>(n); ++j) {
                    if (!form.a[i][j].is_zero()) {
                        choices.emplace_back(j, i);
                    }
                }
            }
            if (choices.empty()) {
                break;
            }
            const auto [e, l] = choices[static_cast<std::size_t>(
                rng.uniform(0, static_cast<int>(choices.size()) - 1))];
            const SlackForm next = pivot(form, e, l);
            // Any assignment of the new nonbasics must solve the old system too.
            std::vector<Frac> assignment(n + m);
            for (int nb : next.nonbasic) {
                assignment[static_cast<std::size_t>(nb)] = Frac(rng.uniform(-3, 3));
            }
            const auto full = complete_assignment(next, assignment);
            const auto old_full = complete_assignment(form, full);
            INFO("round ", round);
            CHECK_EQ(old_full, full);
            CHECK_EQ(value_of(next, full), value_of(form, full));
            form = next;
        }
    }
}

namespace {

long long brute_max_flow_value(int n, const std::vector<FlowEdge>& edges, int s, int t) {
    long long best = std::numeric_limits<long long>::max();
    for (int mask = 0; mask < (1 << n); ++mask) {
        if (!(mask >> s & 1) || (mask >> t & 1)) {
            continue;
        }
        long long cap = 0;
        for (const auto& e : edges) {
            if ((mask >> e.u & 1) && !(mask >> e.v & 1)) {
                cap += e.capacity;
            }
        }
        best = std::min(best, cap);
    }
    return best;
}

}  // namespace

TEST(3, "max flow LP reaches the min cut") {
    Rng rng;
    for (int round = 0; round < 25; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 4));
        const int m = static_cast<int>(rng.uniform(1, 5));
        std::vector<FlowEdge> edges;
        for (int i = 0; i < m; ++i) {
            const int u = static_cast<int>(rng.uniform(0, n - 1));
            int v = static_cast<int>(rng.uniform(0, n - 1));
            if (u == v) {
                v = (v + 1) % n;
            }
            edges.push_back({u, v, rng.uniform(0, 5)});
        }
        const int s = 0, t = n - 1;
        const GeneralLp lp = max_flow_lp(n, edges, s, t);
        REQUIRE_EQ(lp.objective.size(), edges.size());
        const auto form = to_standard(lp);
        const auto answer = solve_standard(form.lp);
        INFO("round ", round);
        REQUIRE(answer.status == Status::Optimal);
        const long long expected = brute_max_flow_value(n, edges, s, t);
        CHECK_EQ(recover_value(form, answer.value), Frac(expected));
        // The recovered variables are a valid flow of that value.
        const auto x = recover_solution(form, answer.x);
        CHECK(general_feasible(lp, x));
        Frac net;
        for (int e = 0; e < m; ++e) {
            if (edges[e].u == s) {
                net += x[e];
            }
            if (edges[e].v == s) {
                net -= x[e];
            }
        }
        CHECK_EQ(net, Frac(expected));
    }
}

TEST(3, "shortest path LP matches Bellman-Ford") {
    Rng rng;
    for (int round = 0; round < 15; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 3));
        std::vector<WeightedArc> arcs;
        // Guarantee reachability from s = 0 with a path, then add random arcs.
        for (int v = 1; v < n; ++v) {
            arcs.push_back({v - 1, v, rng.uniform(1, 4)});
        }
        const int extra = static_cast<int>(rng.uniform(0, 3));
        for (int i = 0; i < extra; ++i) {
            const int u = static_cast<int>(rng.uniform(0, n - 1));
            int v = static_cast<int>(rng.uniform(0, n - 1));
            if (u == v) {
                v = (v + 1) % n;
            }
            arcs.push_back({u, v, rng.uniform(0, 4)});
        }
        const GeneralLp lp = shortest_path_lp(n, arcs, 0);
        REQUIRE_EQ(lp.objective.size(), n);
        const auto form = to_standard(lp);
        const auto answer = solve_standard(form.lp);
        INFO("round ", round);
        REQUIRE(answer.status == Status::Optimal);
        std::vector<long long> dist(n, 1'000'000);
        dist[0] = 0;
        for (int pass = 0; pass < n; ++pass) {
            for (const auto& arc : arcs) {
                dist[arc.to] = std::min(dist[arc.to], dist[arc.from] + arc.weight);
            }
        }
        long long total = 0;
        for (long long d : dist) {
            total += d;
        }
        CHECK_EQ(recover_value(form, answer.value), Frac(total));
        const auto x = recover_solution(form, answer.x);
        CHECK(general_feasible(lp, x));
    }
}

TEST(4, "dual has one variable per primal constraint") {
    const StandardLp lp{{Frac(1), Frac(2), Frac(3)}, {{Frac(1), Frac(0), Frac(2)}}, {Frac(5)}};
    const StandardLp dual = dual_of(lp);
    CHECK_EQ(dual.c.size(), 1);
    CHECK_EQ(dual.A.size(), 3);
    CHECK_EQ(dual.b.size(), 3);
}

TEST(4, "strong duality holds on random LPs") {
    Rng rng;
    for (int round = 0; round < 150; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 3));
        const auto m = static_cast<std::size_t>(rng.uniform(1, 3));
        StandardLp lp;
        lp.c = random_row(rng, n);
        for (std::size_t i = 0; i < m; ++i) {
            lp.A.push_back(random_row(rng, n));
            lp.b.push_back(Frac(rng.uniform(-3, 8)));
        }
        const auto primal = solve_standard(lp);
        const auto dual = solve_standard(dual_of(lp));
        INFO("round ", round);
        if (primal.status == Status::Optimal) {
            REQUIRE(dual.status == Status::Optimal);
            CHECK_EQ(primal.value, -dual.value);
        } else if (primal.status == Status::Unbounded) {
            CHECK(dual.status == Status::Infeasible);
        } else {
            CHECK(dual.status != Status::Optimal);
        }
    }
}
