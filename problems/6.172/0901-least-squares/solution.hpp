#pragma once

#include <span>
#include <vector>

#include "practice.hpp"

struct LineFit {
    double slope = 0;
    double intercept = 0;
    double r_squared = 0;
};

// Stage 1
inline LineFit fit_line(std::span<const double> x, std::span<const double> y) { PRACTICE_TODO(); }

// Stage 2
inline double pearson(std::span<const double> x, std::span<const double> y) { PRACTICE_TODO(); }
inline std::vector<double> fit_multilinear(const std::vector<std::vector<double>>& rows,
                                           std::span<const double> y) {
    PRACTICE_TODO();
}

// Stage 3
enum class Growth { Constant, Logarithmic, Linear, NLogN, Quadratic, Cubic, Exponential };

inline double power_law_exponent(std::span<const double> n, std::span<const double> counts) {
    PRACTICE_TODO();
}
inline Growth classify_growth(std::span<const double> n, std::span<const double> counts) {
    PRACTICE_TODO();
}
