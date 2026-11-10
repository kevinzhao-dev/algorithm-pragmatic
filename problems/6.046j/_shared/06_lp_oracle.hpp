// 06_lp_oracle.hpp — a slow but exact LP checker for tests: enumerates every basic solution.
//
// maximize c·x subject to A x ≤ b and x ≥ 0, over exact fractions. Only use it on tiny LPs:
// the running time is exponential in the number of variables.
#pragma once

#include <optional>
#include <vector>

#include "06_fraction.hpp"

namespace lp_oracle {

enum class Status { Optimal, Infeasible, Unbounded };

struct Answer {
    Status status = Status::Infeasible;
    Frac value;
    std::vector<Frac> x;
};

// Exact Gaussian elimination for a square system; nullopt when singular.
inline std::optional<std::vector<Frac>> solve_square(std::vector<std::vector<Frac>> a,
                                                     std::vector<Frac> b) {
    const std::size_t n = a.size();
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivot = col;
        while (pivot < n && a[pivot][col].is_zero()) {
            ++pivot;
        }
        if (pivot == n) {
            return std::nullopt;
        }
        std::swap(a[pivot], a[col]);
        std::swap(b[pivot], b[col]);
        const Frac lead = a[col][col];
        for (std::size_t j = col; j < n; ++j) {
            a[col][j] /= lead;
        }
        b[col] /= lead;
        for (std::size_t i = 0; i < n; ++i) {
            if (i != col && !a[i][col].is_zero()) {
                const Frac factor = a[i][col];
                for (std::size_t j = col; j < n; ++j) {
                    a[i][j] -= factor * a[col][j];
                }
                b[i] -= factor * b[col];
            }
        }
    }
    return b;
}

namespace detail {

inline bool feasible(const std::vector<std::vector<Frac>>& rows, const std::vector<Frac>& rhs,
                     const std::vector<Frac>& x) {
    for (std::size_t i = 0; i < rows.size(); ++i) {
        Frac lhs;
        for (std::size_t j = 0; j < x.size(); ++j) {
            lhs += rows[i][j] * x[j];
        }
        if (lhs > rhs[i]) {
            return false;
        }
    }
    return true;
}

inline Frac dot(const std::vector<Frac>& a, const std::vector<Frac>& b) {
    Frac out;
    for (std::size_t i = 0; i < a.size(); ++i) {
        out += a[i] * b[i];
    }
    return out;
}

// Best objective over all vertices of {rows x <= rhs}; nullopt if the region has no vertex.
inline std::optional<std::pair<Frac, std::vector<Frac>>> best_vertex(
    const std::vector<std::vector<Frac>>& rows, const std::vector<Frac>& rhs,
    const std::vector<Frac>& c) {
    const std::size_t n = c.size();
    const std::size_t total = rows.size();
    std::optional<std::pair<Frac, std::vector<Frac>>> best;
    std::vector<std::size_t> pick(n);
    for (std::size_t i = 0; i < n; ++i) {
        pick[i] = i;
    }
    while (true) {
        std::vector<std::vector<Frac>> a(n, std::vector<Frac>(n));
        std::vector<Frac> b(n);
        for (std::size_t i = 0; i < n; ++i) {
            a[i] = rows[pick[i]];
            b[i] = rhs[pick[i]];
        }
        if (const auto x = solve_square(std::move(a), std::move(b)); x && feasible(rows, rhs, *x)) {
            const Frac value = dot(c, *x);
            if (!best || value > best->first) {
                best = std::make_pair(value, *x);
            }
        }
        std::size_t i = n;
        while (i > 0 && pick[i - 1] == total - n + (i - 1)) {
            --i;
        }
        if (i == 0) {
            break;
        }
        ++pick[i - 1];
        for (std::size_t j = i; j < n; ++j) {
            pick[j] = pick[j - 1] + 1;
        }
    }
    return best;
}

}  // namespace detail

inline Answer maximize(const std::vector<std::vector<Frac>>& a, const std::vector<Frac>& b,
                       const std::vector<Frac>& c) {
    using detail::best_vertex;
    const std::size_t n = c.size();
    std::vector<std::vector<Frac>> rows = a;
    std::vector<Frac> rhs = b;
    for (std::size_t j = 0; j < n; ++j) {
        std::vector<Frac> row(n);
        row[j] = -1;
        rows.push_back(std::move(row));
        rhs.push_back(0);
    }
    Answer answer;
    const auto best = best_vertex(rows, rhs, c);
    if (!best) {
        return answer;  // no vertex: with x >= 0 that means infeasible
    }
    answer.status = Status::Optimal;
    answer.value = best->first;
    answer.x = best->second;
    // A huge box catches unbounded objectives: the best vertex improves when the box grows.
    std::vector<Frac> row(n, Frac(1));
    rows.push_back(std::move(row));
    rhs.push_back(Frac(1'000'000'000));
    const auto boxed = best_vertex(rows, rhs, c);
    if (boxed && boxed->first > best->first) {
        answer.status = Status::Unbounded;
    }
    return answer;
}

}  // namespace lp_oracle
