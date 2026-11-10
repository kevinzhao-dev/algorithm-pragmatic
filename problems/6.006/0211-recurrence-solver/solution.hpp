#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

struct Growth {
    double exponent;
    int log_power;
};

enum class MasterCase { Leaves, Balanced, Root };

struct MasterResult {
    MasterCase which;
    Growth growth;
};

// Stage 1
inline MasterResult solve_master(double a, double b, double c, int k) { PRACTICE_TODO(); }

// Stage 2
inline std::vector<double> level_work(double a, double b, double c, int k, int depth) {
    PRACTICE_TODO();
}
inline double tree_total(double a, double b, double c, int k, int depth) { PRACTICE_TODO(); }

// Stage 3
struct Bounds {
    Growth lower;
    Growth upper;
    bool tight() const {
        return lower.exponent == upper.exponent && lower.log_power == upper.log_power;
    }
};

inline Bounds bounds_with_big_o(double a, double b, double c, int k) { PRACTICE_TODO(); }

// Stage 4
inline Growth solve_chain(double c, int k) { PRACTICE_TODO(); }
inline std::uint64_t chain_work(std::uint64_t s, int c, std::uint64_t n) { PRACTICE_TODO(); }
