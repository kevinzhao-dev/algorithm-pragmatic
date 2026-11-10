#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "06_fraction.hpp"

struct StandardLp {
    std::vector<Frac> c;
    std::vector<std::vector<Frac>> A;
    std::vector<Frac> b;
};

inline std::vector<std::pair<Frac, Frac>> feasible_vertices_2d(const StandardLp& lp) {
    // Lines: every row a·x = b, plus x = 0 and y = 0, as (a0, a1, rhs).
    std::vector<std::array<Frac, 3>> lines;
    for (std::size_t i = 0; i < lp.A.size(); ++i) {
        lines.push_back({lp.A[i][0], lp.A[i][1], lp.b[i]});
    }
    lines.push_back({Frac(1), Frac(0), Frac(0)});
    lines.push_back({Frac(0), Frac(1), Frac(0)});
    const auto feasible = [&](const Frac& x, const Frac& y) {
        if (x < Frac(0) || y < Frac(0)) {
            return false;
        }
        for (std::size_t i = 0; i < lp.A.size(); ++i) {
            if (lp.A[i][0] * x + lp.A[i][1] * y > lp.b[i]) {
                return false;
            }
        }
        return true;
    };
    std::set<std::pair<Frac, Frac>> out;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        for (std::size_t j = i + 1; j < lines.size(); ++j) {
            const auto& [a1, b1, c1] = lines[i];
            const auto& [a2, b2, c2] = lines[j];
            const Frac det = a1 * b2 - a2 * b1;
            if (det.is_zero()) {
                continue;
            }
            const Frac x = (c1 * b2 - c2 * b1) / det;
            const Frac y = (a1 * c2 - a2 * c1) / det;
            if (feasible(x, y)) {
                out.insert({x, y});
            }
        }
    }
    return {out.begin(), out.end()};
}

enum class LpStatus { Optimal, Infeasible, Unbounded };

struct LpResult {
    LpStatus status = LpStatus::Infeasible;
    Frac value;
    std::vector<Frac> x;
    std::vector<Frac> dual;
};

namespace simplex_detail {

// Dense slack form: x[basic[i]] = b[i] - sum_j a[i][j] x[nonbasic[j]], z = v + sum c[j] x[nonbasic[j]].
struct Tableau {
    std::vector<int> nonbasic, basic;
    std::vector<std::vector<Frac>> a;
    std::vector<Frac> b, c;
    Frac v;
    int total_vars = 0;

    std::vector<Frac> basic_solution() const {
        std::vector<Frac> x(total_vars);
        for (std::size_t i = 0; i < basic.size(); ++i) {
            x[basic[i]] = b[i];
        }
        return x;
    }

    void pivot(int e, int l) {
        const Frac lead = a[l][e];
        const Frac inverse = Frac(1) / lead;
        const int m = static_cast<int>(basic.size());
        const int n = static_cast<int>(nonbasic.size());
        b[l] *= inverse;
        for (int j = 0; j < n; ++j) {
            a[l][j] = j == e ? inverse : a[l][j] * inverse;
        }
        for (int i = 0; i < m; ++i) {
            if (i == l || a[i][e].is_zero()) {
                continue;
            }
            const Frac factor = a[i][e];
            b[i] -= factor * b[l];
            for (int j = 0; j < n; ++j) {
                if (j == e) {
                    a[i][j] = -(factor * a[l][j]);
                } else {
                    a[i][j] -= factor * a[l][j];
                }
            }
        }
        if (!c[e].is_zero()) {
            const Frac factor = c[e];
            v += factor * b[l];
            for (int j = 0; j < n; ++j) {
                if (j == e) {
                    c[j] = -(factor * a[l][j]);
                } else {
                    c[j] -= factor * a[l][j];
                }
            }
        }
        std::swap(nonbasic[e], basic[l]);
    }

    // Runs simplex with Bland's rule; returns false when unbounded.
    bool optimize(std::vector<std::vector<Frac>>* trace, int original_vars) {
        while (true) {
            // Entering variable: smallest variable index with positive reduced cost.
            int e = -1;
            for (int j = 0; j < static_cast<int>(nonbasic.size()); ++j) {
                if (c[j] > Frac(0) && (e == -1 || nonbasic[j] < nonbasic[e])) {
                    e = j;
                }
            }
            if (e == -1) {
                return true;
            }
            // Leaving variable: minimum ratio, ties broken by smallest basic variable index.
            int l = -1;
            Frac best_ratio;
            for (int i = 0; i < static_cast<int>(basic.size()); ++i) {
                if (a[i][e] > Frac(0)) {
                    const Frac ratio = b[i] / a[i][e];
                    if (l == -1 || ratio < best_ratio ||
                        (ratio == best_ratio && basic[i] < basic[l])) {
                        l = i;
                        best_ratio = ratio;
                    }
                }
            }
            if (l == -1) {
                return false;
            }
            pivot(e, l);
            if (trace != nullptr) {
                auto point = basic_solution();
                point.resize(static_cast<std::size_t>(original_vars));
                trace->push_back(std::move(point));
            }
        }
    }
};

inline Tableau make_tableau(const StandardLp& lp) {
    const int n = static_cast<int>(lp.c.size());
    const int m = static_cast<int>(lp.b.size());
    Tableau t;
    t.total_vars = n + m;
    for (int j = 0; j < n; ++j) {
        t.nonbasic.push_back(j);
    }
    for (int i = 0; i < m; ++i) {
        t.basic.push_back(n + i);
    }
    t.a = lp.A;
    t.b = lp.b;
    t.c = lp.c;
    return t;
}

// Phase I: returns a feasible slack form for the original objective, or nullopt.
inline std::optional<Tableau> phase_one(const StandardLp& lp) {
    const int n = static_cast<int>(lp.c.size());
    const int m = static_cast<int>(lp.b.size());
    Tableau t = make_tableau(lp);
    int worst = 0;
    bool need = false;
    for (int i = 0; i < m; ++i) {
        if (t.b[i] < t.b[worst]) {
            worst = i;
        }
        need = need || t.b[i] < Frac(0);
    }
    if (!need) {
        return t;
    }
    // Auxiliary LP: artificial variable x0 = n + m, maximize -x0.
    const int x0 = n + m;
    t.total_vars = n + m + 1;
    t.nonbasic.push_back(x0);
    for (int i = 0; i < m; ++i) {
        t.a[i].push_back(Frac(-1));
    }
    const std::vector<Frac> original_c = t.c;
    t.c.assign(n + 1, Frac(0));
    t.c[n] = Frac(-1);
    t.pivot(n, worst);
    t.optimize(nullptr, 0);
    // Infeasible unless x0 can be made zero.
    Frac x0_value;
    for (std::size_t i = 0; i < t.basic.size(); ++i) {
        if (t.basic[i] == x0) {
            x0_value = t.b[i];
        }
    }
    if (!x0_value.is_zero()) {
        return std::nullopt;
    }
    // Pivot x0 out of the basis if necessary (degenerate), then delete its column. If x0 stays
    // basic because its whole row is zero, the row is redundant and can be dropped instead.
    for (int i = 0; i < m; ++i) {
        if (t.basic[i] == x0) {
            int e = -1;
            for (int j = 0; j < static_cast<int>(t.nonbasic.size()); ++j) {
                if (!t.a[i][j].is_zero() && (e == -1 || t.nonbasic[j] < t.nonbasic[e])) {
                    e = j;
                }
            }
            if (e != -1) {
                t.pivot(e, i);
            } else {
                t.basic.erase(t.basic.begin() + i);
                t.a.erase(t.a.begin() + i);
                t.b.erase(t.b.begin() + i);
            }
            break;
        }
    }
    int column = -1;
    for (int j = 0; j < static_cast<int>(t.nonbasic.size()); ++j) {
        if (t.nonbasic[j] == x0) {
            column = j;
        }
    }
    if (column != -1) {
        t.nonbasic.erase(t.nonbasic.begin() + column);
        for (auto& row : t.a) {
            row.erase(row.begin() + column);
        }
    }
    t.total_vars = n + m;
    // Restore the true objective, expressed over the current nonbasic variables.
    t.c.assign(t.nonbasic.size(), Frac(0));
    t.v = Frac(0);
    for (int var = 0; var < n; ++var) {
        const Frac coeff = original_c[var];
        if (coeff.is_zero()) {
            continue;
        }
        const auto pos = std::find(t.nonbasic.begin(), t.nonbasic.end(), var);
        if (pos != t.nonbasic.end()) {
            t.c[static_cast<std::size_t>(pos - t.nonbasic.begin())] += coeff;
            continue;
        }
        const auto row_pos = std::find(t.basic.begin(), t.basic.end(), var);
        const auto i = static_cast<std::size_t>(row_pos - t.basic.begin());
        t.v += coeff * t.b[i];
        for (std::size_t j = 0; j < t.nonbasic.size(); ++j) {
            t.c[j] -= coeff * t.a[i][j];
        }
    }
    return t;
}

}  // namespace simplex_detail

inline LpResult solve_lp(const StandardLp& lp, std::vector<std::vector<Frac>>* trace = nullptr) {
    using simplex_detail::Tableau;
    const int n = static_cast<int>(lp.c.size());
    const int m = static_cast<int>(lp.b.size());
    LpResult result;
    auto start = simplex_detail::phase_one(lp);
    if (!start) {
        return result;
    }
    Tableau t = std::move(*start);
    if (trace != nullptr) {
        auto point = t.basic_solution();
        point.resize(static_cast<std::size_t>(n));
        trace->push_back(std::move(point));
    }
    if (!t.optimize(trace, n)) {
        result.status = LpStatus::Unbounded;
        return result;
    }
    result.status = LpStatus::Optimal;
    result.value = t.v;
    auto x = t.basic_solution();
    x.resize(static_cast<std::size_t>(n));
    result.x = std::move(x);
    result.dual.assign(static_cast<std::size_t>(m), Frac(0));
    for (std::size_t j = 0; j < t.nonbasic.size(); ++j) {
        if (t.nonbasic[j] >= n) {
            result.dual[static_cast<std::size_t>(t.nonbasic[j] - n)] = -t.c[j];
        }
    }
    return result;
}
