#pragma once

#include <limits>
#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

inline constexpr long long kInf = std::numeric_limits<long long>::max() / 4;
using Matrix = std::vector<std::vector<long long>>;

// Stage 1
inline Matrix min_plus(const Matrix& a, const Matrix& b) { PRACTICE_TODO(); }
inline std::optional<Matrix> apsp_repeated_squaring(const Matrix& w) { PRACTICE_TODO(); }

// Stage 2
class FloydWarshall {
public:
    explicit FloydWarshall(Matrix w) { PRACTICE_TODO(); }

    bool has_negative_cycle() const { PRACTICE_TODO(); }
    long long dist(int u, int v) const { PRACTICE_TODO(); }
    std::vector<int> path(int u, int v) const { PRACTICE_TODO(); }
};

// Stage 3
struct Edge {
    int from = 0;
    int to = 0;
    long long weight = 0;
};
inline std::optional<Matrix> johnson(int n, std::span<const Edge> edges) { PRACTICE_TODO(); }

// Stage 4
struct Constraint {
    int later = 0;
    int earlier = 0;
    long long bound = 0;
};
inline std::optional<std::vector<long long>> solve_difference_constraints(
    int n, std::span<const Constraint> constraints) {
    PRACTICE_TODO();
}
