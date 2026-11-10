#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

struct LineFit {
    double slope = 0;
    double intercept = 0;
    double r_squared = 0;
};

inline LineFit fit_line(std::span<const double> x, std::span<const double> y) {
    if (x.size() != y.size() || x.size() < 2) {
        throw std::invalid_argument("need equal-length series of length >= 2");
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
        throw std::invalid_argument("all x values are equal");
    }
    LineFit fit;
    fit.slope = (n * sum_xy - sum_x * sum_y) / denominator;
    fit.intercept = (sum_y - fit.slope * sum_x) / n;
    const double mean_y = sum_y / n;
    double ss_res = 0, ss_tot = 0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        const double error = y[i] - (fit.slope * x[i] + fit.intercept);
        ss_res += error * error;
        ss_tot += (y[i] - mean_y) * (y[i] - mean_y);
    }
    fit.r_squared = ss_tot == 0 ? 1.0 : 1.0 - ss_res / ss_tot;
    return fit;
}

inline double pearson(std::span<const double> x, std::span<const double> y) {
    if (x.size() != y.size() || x.size() < 2) {
        throw std::invalid_argument("need equal-length series of length >= 2");
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

inline std::vector<double> fit_multilinear(const std::vector<std::vector<double>>& rows,
                                           std::span<const double> y) {
    if (rows.empty() || rows.size() != y.size() || rows.front().empty()) {
        throw std::invalid_argument("need one non-empty row per observation");
    }
    const std::size_t k = rows.front().size();
    std::vector<double> scale(k, 0.0);
    for (const auto& row : rows) {
        if (row.size() != k) {
            throw std::invalid_argument("rows have different lengths");
        }
        for (std::size_t j = 0; j < k; ++j) {
            scale[j] = std::max(scale[j], std::fabs(row[j]));
        }
    }
    for (const double s : scale) {
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

enum class Growth { Constant, Logarithmic, Linear, NLogN, Quadratic, Cubic, Exponential };

inline double power_law_exponent(std::span<const double> n, std::span<const double> counts) {
    if (n.size() != counts.size() || n.size() < 2) {
        throw std::invalid_argument("need equal-length series of length >= 2");
    }
    std::vector<double> log_n(n.size()), log_c(n.size());
    for (std::size_t i = 0; i < n.size(); ++i) {
        log_n[i] = std::log(n[i]);
        log_c[i] = std::log(counts[i]);
    }
    return fit_line(log_n, log_c).slope;
}

inline Growth classify_growth(std::span<const double> n, std::span<const double> counts) {
    if (n.size() != counts.size() || n.size() < 3) {
        throw std::invalid_argument("need equal-length series of length >= 3");
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
    const LineFit exponential = fit_line(n, log_c);
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
