#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

inline std::optional<std::uint64_t> checked_sum_of_cubes(std::uint64_t n) {
    // (n(n+1)/2)^2 fits iff n(n+1)/2 <= 2^32 - 1, which is exactly n <= 92681.
    if (n > 92'681) {
        return std::nullopt;
    }
    const std::uint64_t triangular = n * (n + 1) / 2;
    return triangular * triangular;
}

inline std::uint64_t sum_of_cubes_mod(std::uint64_t n, std::uint64_t m) {
    using Wide = unsigned __int128;
    const Wide triangular = (n % 2 == 0) ? Wide{n / 2} * (n + 1) : Wide{(n + 1) / 2} * n;
    const Wide reduced = triangular % m;
    return static_cast<std::uint64_t>(reduced * reduced % m);
}

class Poly {
public:
    Poly() = default;
    explicit Poly(std::vector<std::int64_t> coefficients) : coeffs_(std::move(coefficients)) {
        normalize();
    }

    const std::vector<std::int64_t>& coefficients() const { return coeffs_; }
    int degree() const { return static_cast<int>(coeffs_.size()) - 1; }

    std::int64_t operator()(std::int64_t x) const {
        std::int64_t value = 0;
        for (auto it = coeffs_.rbegin(); it != coeffs_.rend(); ++it) {
            value = value * x + *it;
        }
        return value;
    }

    Poly shifted() const {
        // Repeated synthetic division by (x - 1) turns p(x) into the coefficients of p(x + 1).
        std::vector<std::int64_t> c = coeffs_;
        for (std::size_t i = 0; i + 1 < c.size(); ++i) {
            for (std::size_t j = c.size() - 1; j > i; --j) {
                c[j - 1] += c[j];
            }
        }
        return Poly(std::move(c));
    }

    friend Poly operator+(const Poly& a, const Poly& b) { return combine(a, b, 1); }
    friend Poly operator-(const Poly& a, const Poly& b) { return combine(a, b, -1); }

    friend Poly operator*(const Poly& a, const Poly& b) {
        if (a.coeffs_.empty() || b.coeffs_.empty()) {
            return Poly{};
        }
        std::vector<std::int64_t> product(a.coeffs_.size() + b.coeffs_.size() - 1, 0);
        for (std::size_t i = 0; i < a.coeffs_.size(); ++i) {
            for (std::size_t j = 0; j < b.coeffs_.size(); ++j) {
                product[i + j] += a.coeffs_[i] * b.coeffs_[j];
            }
        }
        return Poly(std::move(product));
    }

    friend bool operator==(const Poly& a, const Poly& b) { return a.coeffs_ == b.coeffs_; }

private:
    static Poly combine(const Poly& a, const Poly& b, std::int64_t sign) {
        std::vector<std::int64_t> result(std::max(a.coeffs_.size(), b.coeffs_.size()), 0);
        std::copy(a.coeffs_.begin(), a.coeffs_.end(), result.begin());
        for (std::size_t i = 0; i < b.coeffs_.size(); ++i) {
            result[i] += sign * b.coeffs_[i];
        }
        return Poly(std::move(result));
    }

    void normalize() {
        while (!coeffs_.empty() && coeffs_.back() == 0) {
            coeffs_.pop_back();
        }
    }

    std::vector<std::int64_t> coeffs_;
};

struct InductionReport {
    bool base_case = false;
    bool inductive_step = false;
    bool proved() const { return base_case && inductive_step; }
};

inline InductionReport check_sum_formula(const Poly& term, const Poly& numerator,
                                         std::int64_t denominator) {
    if (denominator == 0) {
        throw std::invalid_argument("zero denominator");
    }
    InductionReport report;
    report.base_case = numerator(1) == denominator * term(1);
    const Poly next_numerator = numerator.shifted();
    report.inductive_step =
        next_numerator - numerator == Poly({denominator}) * term.shifted();
    return report;
}
