#pragma once

#include <utility>
#include <vector>

#include "06_fraction.hpp"
#include "practice.hpp"

struct StandardLp {
    std::vector<Frac> c;
    std::vector<std::vector<Frac>> A;
    std::vector<Frac> b;
};

// Stage 1
inline std::vector<std::pair<Frac, Frac>> feasible_vertices_2d(const StandardLp& lp) {
    PRACTICE_TODO();
}

enum class LpStatus { Optimal, Infeasible, Unbounded };

struct LpResult {
    LpStatus status = LpStatus::Infeasible;
    Frac value;
    std::vector<Frac> x;
    std::vector<Frac> dual;
};

// Stages 2-4
inline LpResult solve_lp(const StandardLp& lp, std::vector<std::vector<Frac>>* trace = nullptr) {
    PRACTICE_TODO();
}
