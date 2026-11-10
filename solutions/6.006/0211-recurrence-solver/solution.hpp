#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct Growth {
    double exponent;
    int log_power;
};

enum class MasterCase { Leaves, Balanced, Root };

struct MasterResult {
    MasterCase which;
    Growth growth;
};

inline constexpr double kExponentTolerance = 1e-9;

inline MasterResult solve_master(double a, double b, double c, int k) {
    if (!(a >= 1.0) || !(b > 1.0) || !(c >= 0.0) || k < 0) {
        throw std::invalid_argument("invalid recurrence parameters");
    }
    const double e = std::log(a) / std::log(b);
    if (std::abs(c - e) <= kExponentTolerance) {
        return {MasterCase::Balanced, Growth{c, k + 1}};
    }
    if (c < e) {
        return {MasterCase::Leaves, Growth{e, 0}};
    }
    return {MasterCase::Root, Growth{c, k}};
}

inline std::vector<double> level_work(double a, double b, double c, int k, int depth) {
    std::vector<double> levels;
    levels.reserve(static_cast<std::size_t>(depth) + 1);
    double subproblems = 1.0;
    for (int level = 0; level < depth; ++level) {
        const double size = std::pow(b, depth - level);
        levels.push_back(subproblems * std::pow(size, c) * std::pow(std::log2(size), k));
        subproblems *= a;
    }
    levels.push_back(subproblems);
    return levels;
}

inline double tree_total(double a, double b, double c, int k, int depth) {
    double total = 0.0;
    for (const double work : level_work(a, b, c, k, depth)) {
        total += work;
    }
    return total;
}

struct Bounds {
    Growth lower;
    Growth upper;
    bool tight() const {
        return lower.exponent == upper.exponent && lower.log_power == upper.log_power;
    }
};

inline Bounds bounds_with_big_o(double a, double b, double c, int k) {
    const MasterResult result = solve_master(a, b, c, k);
    const double e = std::log(a) / std::log(b);
    return {Growth{e, 0}, result.growth};
}

inline Growth solve_chain(double c, int k) {
    if (!(c >= 0.0) || k < 0) {
        throw std::invalid_argument("invalid chain parameters");
    }
    return {c + 1.0, k};
}

inline std::uint64_t chain_work(std::uint64_t s, int c, std::uint64_t n) {
    std::uint64_t total = 0;
    while (n >= 1) {
        std::uint64_t term = 1;
        for (int i = 0; i < c; ++i) {
            term *= n;
        }
        total += term;
        if (n <= s) {
            break;
        }
        n -= s;
    }
    return total;
}
