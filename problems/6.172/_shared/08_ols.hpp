// SPOILER: complete least-squares routines, which answer 6.172/0901. Finish 0901 first.
//
// 08_ols.hpp — ordinary least squares, shared by the 6.172 dynamic-analysis exercises.
#pragma once

#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace practice::ols {

struct Line {
    double slope = 0;
    double intercept = 0;
    double r_squared = 0;
};

// Fits y = slope * x + intercept. Needs at least two distinct x values.
inline Line fit_line(std::span<const double> x, std::span<const double> y) {
    if (x.size() != y.size() || x.size() < 2) {
        throw std::invalid_argument("fit_line needs two equal-length series of length >= 2");
    }
    const auto n = static_cast<double>(x.size());
    double sum_x = 0, sum_y = 0, sum_xx = 0, sum_xy = 0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        sum_x += x[i];
        sum_y += y[i];
        sum_xx += x[i] * x[i];
        sum_xy += x[i] * y[i];
    }
    const double denominator = n * sum_xx - sum_x * sum_x;
    if (denominator == 0) {
        throw std::invalid_argument("fit_line needs two distinct x values");
    }
    Line fit;
    fit.slope = (n * sum_xy - sum_x * sum_y) / denominator;
    fit.intercept = (sum_y - fit.slope * sum_x) / n;
    double ss_res = 0, ss_tot = 0;
    const double mean_y = sum_y / n;
    for (std::size_t i = 0; i < x.size(); ++i) {
        const double predicted = fit.slope * x[i] + fit.intercept;
        ss_res += (y[i] - predicted) * (y[i] - predicted);
        ss_tot += (y[i] - mean_y) * (y[i] - mean_y);
    }
    fit.r_squared = ss_tot == 0 ? 1.0 : 1.0 - ss_res / ss_tot;
    return fit;
}

// Pearson correlation; returns 0 when either series is constant.
inline double correlation(std::span<const double> x, std::span<const double> y) {
    if (x.size() != y.size() || x.empty()) {
        throw std::invalid_argument("correlation needs two equal-length non-empty series");
    }
    const auto n = static_cast<double>(x.size());
    double mean_x = 0, mean_y = 0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        mean_x += x[i];
        mean_y += y[i];
    }
    mean_x /= n;
    mean_y /= n;
    double sxx = 0, syy = 0, sxy = 0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        sxx += (x[i] - mean_x) * (x[i] - mean_x);
        syy += (y[i] - mean_y) * (y[i] - mean_y);
        sxy += (x[i] - mean_x) * (y[i] - mean_y);
    }
    if (sxx == 0 || syy == 0) {
        return 0;
    }
    return sxy / std::sqrt(sxx * syy);
}

// Least squares for y ≈ rows * beta (no implicit intercept), via the normal equations and
// Gaussian elimination with partial pivoting. Columns are scaled first because counts of very
// different magnitudes would otherwise make the pivot test meaningless. Throws
// std::domain_error when the regressors are linearly dependent.
inline std::vector<double> solve(const std::vector<std::vector<double>>& rows,
                                 std::span<const double> y) {
    if (rows.empty() || rows.size() != y.size() || rows.front().empty()) {
        throw std::invalid_argument("solve needs one non-empty row of regressors per observation");
    }
    const std::size_t k = rows.front().size();
    std::vector<double> scale(k, 0.0);
    for (const auto& row : rows) {
        if (row.size() != k) {
            throw std::invalid_argument("all rows must have the same number of regressors");
        }
        for (std::size_t j = 0; j < k; ++j) {
            scale[j] = std::max(scale[j], std::fabs(row[j]));
        }
    }
    for (double& s : scale) {
        if (s == 0) {
            throw std::domain_error("a regressor is identically zero");
        }
    }

    std::vector<std::vector<double>> normal(k, std::vector<double>(k + 1, 0.0));
    for (std::size_t r = 0; r < rows.size(); ++r) {
        for (std::size_t i = 0; i < k; ++i) {
            const double xi = rows[r][i] / scale[i];
            for (std::size_t j = 0; j < k; ++j) {
                normal[i][j] += xi * rows[r][j] / scale[j];
            }
            normal[i][k] += xi * y[r];
        }
    }

    for (std::size_t col = 0; col < k; ++col) {
        std::size_t pivot = col;
        for (std::size_t row = col + 1; row < k; ++row) {
            if (std::fabs(normal[row][col]) > std::fabs(normal[pivot][col])) {
                pivot = row;
            }
        }
        if (std::fabs(normal[pivot][col]) < 1e-12 * static_cast<double>(rows.size())) {
            throw std::domain_error("regressors are linearly dependent");
        }
        std::swap(normal[col], normal[pivot]);
        for (std::size_t row = col + 1; row < k; ++row) {
            const double factor = normal[row][col] / normal[col][col];
            for (std::size_t j = col; j <= k; ++j) {
                normal[row][j] -= factor * normal[col][j];
            }
        }
    }

    std::vector<double> beta(k);
    for (std::size_t i = k; i-- > 0;) {
        double value = normal[i][k];
        for (std::size_t j = i + 1; j < k; ++j) {
            value -= normal[i][j] * beta[j];
        }
        beta[i] = value / normal[i][i];
    }
    for (std::size_t i = 0; i < k; ++i) {
        beta[i] /= scale[i];
    }
    return beta;
}

// Slope of log(counts) against log(n): the exponent k of a power law counts ≈ a * n^k.
inline double power_law_exponent(std::span<const double> n, std::span<const double> counts) {
    if (n.size() != counts.size() || n.size() < 2) {
        throw std::invalid_argument("power_law_exponent needs equal-length series of length >= 2");
    }
    std::vector<double> log_n(n.size()), log_c(n.size());
    for (std::size_t i = 0; i < n.size(); ++i) {
        log_n[i] = std::log(n[i]);
        log_c[i] = std::log(counts[i]);
    }
    return fit_line(log_n, log_c).slope;
}

enum class Growth { Constant, Logarithmic, Linear, NLogN, Quadratic, Cubic, Exponential };

inline const char* growth_name(Growth growth) {
    switch (growth) {
        case Growth::Constant:
            return "Constant";
        case Growth::Logarithmic:
            return "Logarithmic";
        case Growth::Linear:
            return "Linear";
        case Growth::NLogN:
            return "NLogN";
        case Growth::Quadratic:
            return "Quadratic";
        case Growth::Cubic:
            return "Cubic";
        case Growth::Exponential:
            return "Exponential";
    }
    return "?";
}

// Picks the growth class whose fit counts ≈ a * f(n) leaves the smallest RMS error on the log
// scale; the exponential family gets its own two-parameter fit of log(counts) against n.
inline Growth classify_growth(std::span<const double> n, std::span<const double> counts) {
    if (n.size() != counts.size() || n.size() < 3) {
        throw std::invalid_argument("classify_growth needs equal-length series of length >= 3");
    }
    const auto rms_for = [&](auto&& f) {
        double numerator = 0, denominator = 0;
        for (std::size_t i = 0; i < n.size(); ++i) {
            numerator += counts[i] * f(n[i]);
            denominator += f(n[i]) * f(n[i]);
        }
        const double a = numerator / denominator;
        double error = 0;
        for (std::size_t i = 0; i < n.size(); ++i) {
            const double r = std::log(counts[i] / (a * f(n[i])));
            error += r * r;
        }
        return std::sqrt(error / static_cast<double>(n.size()));
    };

    struct Candidate {
        Growth growth;
        double rms;
    };
    std::vector<Candidate> candidates = {
        {Growth::Constant, rms_for([](double) { return 1.0; })},
        {Growth::Logarithmic, rms_for([](double x) { return std::log2(x); })},
        {Growth::Linear, rms_for([](double x) { return x; })},
        {Growth::NLogN, rms_for([](double x) { return x * std::log2(x); })},
        {Growth::Quadratic, rms_for([](double x) { return x * x; })},
        {Growth::Cubic, rms_for([](double x) { return x * x * x; })},
    };

    std::vector<double> log_c(n.size());
    for (std::size_t i = 0; i < n.size(); ++i) {
        log_c[i] = std::log(counts[i]);
    }
    const Line exponential = fit_line(n, log_c);
    double error = 0;
    for (std::size_t i = 0; i < n.size(); ++i) {
        const double r = log_c[i] - (exponential.slope * n[i] + exponential.intercept);
        error += r * r;
    }
    candidates.push_back({Growth::Exponential, std::sqrt(error / static_cast<double>(n.size()))});

    Growth best = candidates.front().growth;
    double best_rms = std::numeric_limits<double>::infinity();
    for (const Candidate& candidate : candidates) {
        if (candidate.rms < best_rms - 1e-9) {
            best_rms = candidate.rms;
            best = candidate.growth;
        }
    }
    return best;
}

}  // namespace practice::ols
