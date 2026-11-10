#pragma once

#include <span>
#include <vector>

#include "06_fraction.hpp"
#include "practice.hpp"

// Stage 1
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
    // Add whatever you need here to recover the original solution.
};

inline StandardForm to_standard(const GeneralLp& lp) { PRACTICE_TODO(); }
inline std::vector<Frac> recover_solution(const StandardForm& form, std::span<const Frac> x) {
    PRACTICE_TODO();
}
inline Frac recover_value(const StandardForm& form, const Frac& standard_value) { PRACTICE_TODO(); }

// Stage 2
struct SlackForm {
    std::vector<int> nonbasic, basic;
    std::vector<std::vector<Frac>> a;
    std::vector<Frac> b;
    std::vector<Frac> c;
    Frac v;
};
inline SlackForm to_slack(const StandardLp& lp) { PRACTICE_TODO(); }
inline SlackForm pivot(const SlackForm& form, int e, int l) { PRACTICE_TODO(); }

// Stage 3
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
    PRACTICE_TODO();
}
inline GeneralLp shortest_path_lp(int n, std::span<const WeightedArc> arcs, int s) {
    PRACTICE_TODO();
}

// Stage 4
inline StandardLp dual_of(const StandardLp& lp) { PRACTICE_TODO(); }
