#pragma once

#include <limits>
#include <vector>

#include "practice.hpp"

inline constexpr long long kInf = std::numeric_limits<long long>::max() / 4;
using Matrix = std::vector<std::vector<long long>>;

// Stage 1
class DynamicApsp {
public:
    explicit DynamicApsp(Matrix w) { PRACTICE_TODO(); }

    long long dist(int u, int v) const { PRACTICE_TODO(); }
    std::vector<int> path(int u, int v) const { PRACTICE_TODO(); }
    long long update(int i, int j, long long r) { PRACTICE_TODO(); }
};

// Stage 2
struct Scenario {
    Matrix w;
    int i = 0;
    int j = 0;
    long long r = 0;
};
inline Scenario worst_update_scenario(int n) { PRACTICE_TODO(); }

// Stage 3
inline Matrix bounded_hop_floyd(const Matrix& w, int h) { PRACTICE_TODO(); }

// Stage 4
inline Matrix bounded_hop_doubling(const Matrix& w, int h) { PRACTICE_TODO(); }

// Stage 5
class DynamicBoundedApsp {
public:
    DynamicBoundedApsp(Matrix w, int h) { PRACTICE_TODO(); }

    long long dist(int u, int v) const { PRACTICE_TODO(); }
    long long dist_at_most(int u, int v, int k) const { PRACTICE_TODO(); }
    void update(int i, int j, long long r) { PRACTICE_TODO(); }
};
