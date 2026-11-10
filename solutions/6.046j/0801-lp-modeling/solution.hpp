#pragma once

#include <span>
#include <vector>

#include "06_fraction.hpp"

enum class Relation { LessEq, GreaterEq, Equal };

struct GeneralConstraint {
    std::vector<Frac> coeffs;
    Relation relation = Relation::LessEq;
    Frac rhs;
};

struct GeneralLp {
    bool maximize = true;
    std::vector<Frac> objective;
    std::vector<GeneralConstraint> constraints;
    std::vector<bool> nonnegative;
};

struct StandardLp {
    std::vector<Frac> c;
    std::vector<std::vector<Frac>> A;
    std::vector<Frac> b;
};

struct StandardForm {
    StandardLp lp;
    bool negated_objective = false;
    std::vector<int> positive_column;
    std::vector<int> negative_column;  // -1 when the original variable is nonnegative
};

inline StandardForm to_standard(const GeneralLp& lp) {
    StandardForm form;
    form.negated_objective = !lp.maximize;
    const std::size_t n = lp.objective.size();
    int columns = 0;
    form.positive_column.resize(n);
    form.negative_column.assign(n, -1);
    for (std::size_t j = 0; j < n; ++j) {
        form.positive_column[j] = columns++;
        if (!lp.nonnegative[j]) {
            form.negative_column[j] = columns++;
        }
    }
    const auto widen = [&](const std::vector<Frac>& original, Frac sign) {
        std::vector<Frac> row(columns);
        for (std::size_t j = 0; j < n; ++j) {
            row[form.positive_column[j]] = sign * original[j];
            if (form.negative_column[j] != -1) {
                row[form.negative_column[j]] = -(sign * original[j]);
            }
        }
        return row;
    };
    form.lp.c = widen(lp.objective, lp.maximize ? Frac(1) : Frac(-1));
    const auto emit = [&](const std::vector<Frac>& coeffs, Frac rhs, Frac sign) {
        form.lp.A.push_back(widen(coeffs, sign));
        form.lp.b.push_back(sign * rhs);
    };
    for (const auto& constraint : lp.constraints) {
        if (constraint.relation != Relation::GreaterEq) {
            emit(constraint.coeffs, constraint.rhs, Frac(1));
        }
        if (constraint.relation != Relation::LessEq) {
            emit(constraint.coeffs, constraint.rhs, Frac(-1));
        }
    }
    return form;
}

inline std::vector<Frac> recover_solution(const StandardForm& form, std::span<const Frac> x) {
    std::vector<Frac> out(form.positive_column.size());
    for (std::size_t j = 0; j < out.size(); ++j) {
        out[j] = x[form.positive_column[j]];
        if (form.negative_column[j] != -1) {
            out[j] -= x[form.negative_column[j]];
        }
    }
    return out;
}

inline Frac recover_value(const StandardForm& form, const Frac& standard_value) {
    return form.negated_objective ? -standard_value : standard_value;
}

struct SlackForm {
    std::vector<int> nonbasic, basic;
    std::vector<std::vector<Frac>> a;
    std::vector<Frac> b;
    std::vector<Frac> c;
    Frac v;
};

inline SlackForm to_slack(const StandardLp& lp) {
    const int n = static_cast<int>(lp.c.size());
    const int m = static_cast<int>(lp.b.size());
    SlackForm form;
    for (int j = 0; j < n; ++j) {
        form.nonbasic.push_back(j);
    }
    for (int i = 0; i < m; ++i) {
        form.basic.push_back(n + i);
    }
    form.a = lp.A;
    form.b = lp.b;
    form.c = lp.c;
    return form;
}

inline SlackForm pivot(const SlackForm& form, int e, int l) {
    const int m = static_cast<int>(form.basic.size());
    const int n = static_cast<int>(form.nonbasic.size());
    SlackForm out = form;
    const Frac lead = form.a[l][e];
    const Frac inverse = Frac(1) / lead;
    // Rewrite the leaving equation so that the entering variable is on the left.
    out.b[l] = form.b[l] * inverse;
    for (int j = 0; j < n; ++j) {
        out.a[l][j] = j == e ? inverse : form.a[l][j] * inverse;
    }
    for (int i = 0; i < m; ++i) {
        if (i == l) {
            continue;
        }
        const Frac factor = form.a[i][e];
        out.b[i] = form.b[i] - factor * out.b[l];
        for (int j = 0; j < n; ++j) {
            if (j == e) {
                out.a[i][j] = -(factor * out.a[l][j]);
            } else {
                out.a[i][j] = form.a[i][j] - factor * out.a[l][j];
            }
        }
    }
    const Frac entering_cost = form.c[e];
    out.v = form.v + entering_cost * out.b[l];
    for (int j = 0; j < n; ++j) {
        if (j == e) {
            out.c[j] = -(entering_cost * out.a[l][j]);
        } else {
            out.c[j] = form.c[j] - entering_cost * out.a[l][j];
        }
    }
    out.nonbasic[e] = form.basic[l];
    out.basic[l] = form.nonbasic[e];
    return out;
}

struct FlowEdge {
    int u = 0;
    int v = 0;
    long long capacity = 0;
};

struct WeightedArc {
    int from = 0;
    int to = 0;
    long long weight = 0;
};

inline GeneralLp max_flow_lp(int n, std::span<const FlowEdge> edges, int s, int t) {
    const std::size_t m = edges.size();
    GeneralLp lp;
    lp.maximize = true;
    lp.objective.assign(m, Frac(0));
    lp.nonnegative.assign(m, true);
    for (std::size_t e = 0; e < m; ++e) {
        if (edges[e].u == s) {
            lp.objective[e] += Frac(1);
        }
        if (edges[e].v == s) {
            lp.objective[e] -= Frac(1);
        }
        std::vector<Frac> row(m);
        row[e] = Frac(1);
        lp.constraints.push_back({std::move(row), Relation::LessEq, Frac(edges[e].capacity)});
    }
    for (int x = 0; x < n; ++x) {
        if (x == s || x == t) {
            continue;
        }
        std::vector<Frac> row(m);
        for (std::size_t e = 0; e < m; ++e) {
            if (edges[e].u == x) {
                row[e] += Frac(1);
            }
            if (edges[e].v == x) {
                row[e] -= Frac(1);
            }
        }
        lp.constraints.push_back({std::move(row), Relation::Equal, Frac(0)});
    }
    return lp;
}

inline GeneralLp shortest_path_lp(int n, std::span<const WeightedArc> arcs, int s) {
    GeneralLp lp;
    lp.maximize = true;
    lp.objective.assign(n, Frac(1));
    lp.nonnegative.assign(n, false);
    for (const auto& arc : arcs) {
        std::vector<Frac> row(n);
        row[arc.to] += Frac(1);
        row[arc.from] -= Frac(1);
        lp.constraints.push_back({std::move(row), Relation::LessEq, Frac(arc.weight)});
    }
    std::vector<Frac> row(n);
    row[s] = Frac(1);
    lp.constraints.push_back({std::move(row), Relation::Equal, Frac(0)});
    return lp;
}

inline StandardLp dual_of(const StandardLp& lp) {
    const std::size_t m = lp.b.size();
    const std::size_t n = lp.c.size();
    StandardLp dual;
    dual.c.resize(m);
    for (std::size_t i = 0; i < m; ++i) {
        dual.c[i] = -lp.b[i];  // minimize b·y  ->  maximize -b·y
    }
    dual.A.assign(n, std::vector<Frac>(m));
    dual.b.resize(n);
    for (std::size_t j = 0; j < n; ++j) {
        for (std::size_t i = 0; i < m; ++i) {
            dual.A[j][i] = -lp.A[i][j];  // A^T y >= c  ->  -A^T y <= -c
        }
        dual.b[j] = -lp.c[j];
    }
    return dual;
}
