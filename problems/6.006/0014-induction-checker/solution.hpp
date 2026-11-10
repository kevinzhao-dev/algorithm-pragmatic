#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

// Stage 1
inline std::optional<std::uint64_t> checked_sum_of_cubes(std::uint64_t n) { PRACTICE_TODO(); }
inline std::uint64_t sum_of_cubes_mod(std::uint64_t n, std::uint64_t m) { PRACTICE_TODO(); }

// Stage 2
class Poly {
public:
    Poly() = default;
    explicit Poly(std::vector<std::int64_t> coefficients) { PRACTICE_TODO(); }

    const std::vector<std::int64_t>& coefficients() const { PRACTICE_TODO(); }
    int degree() const { PRACTICE_TODO(); }
    std::int64_t operator()(std::int64_t x) const { PRACTICE_TODO(); }
    Poly shifted() const { PRACTICE_TODO(); }

    friend Poly operator+(const Poly& a, const Poly& b) { PRACTICE_TODO(); }
    friend Poly operator-(const Poly& a, const Poly& b) { PRACTICE_TODO(); }
    friend Poly operator*(const Poly& a, const Poly& b) { PRACTICE_TODO(); }
    friend bool operator==(const Poly& a, const Poly& b) { PRACTICE_TODO(); }
};

// Stage 3
struct InductionReport {
    bool base_case = false;
    bool inductive_step = false;
    bool proved() const { return base_case && inductive_step; }
};

inline InductionReport check_sum_formula(const Poly& term, const Poly& numerator,
                                         std::int64_t denominator) {
    PRACTICE_TODO();
}
