#pragma once

#include <compare>
#include <concepts>
#include <cstdint>
#include <map>
#include <ostream>
#include <utility>
#include <vector>

#include "practice.hpp"

// Stage 1
class Fraction {
public:
    Fraction() { PRACTICE_TODO(); }
    Fraction(std::int64_t numerator, std::int64_t denominator = 1) { PRACTICE_TODO(); }

    std::int64_t numerator() const { PRACTICE_TODO(); }
    std::int64_t denominator() const { PRACTICE_TODO(); }

    friend Fraction operator+(const Fraction& a, const Fraction& b) { PRACTICE_TODO(); }
    friend Fraction operator-(const Fraction& a, const Fraction& b) { PRACTICE_TODO(); }
    friend Fraction operator*(const Fraction& a, const Fraction& b) { PRACTICE_TODO(); }
    friend Fraction operator/(const Fraction& a, const Fraction& b) { PRACTICE_TODO(); }

    friend bool operator==(const Fraction& a, const Fraction& b) { PRACTICE_TODO(); }
    friend std::strong_ordering operator<=>(const Fraction& a, const Fraction& b) {
        PRACTICE_TODO();
    }

    friend std::ostream& operator<<(std::ostream& os, const Fraction& f) {
        return os << f.numerator() << '/' << f.denominator();
    }
};

// Stage 2
template <typename T>
class Pmf {
public:
    explicit Pmf(const std::vector<std::pair<T, Fraction>>& outcomes) { PRACTICE_TODO(); }
    static Pmf uniform(const std::vector<T>& outcomes) { PRACTICE_TODO(); }

    std::vector<T> support() const { PRACTICE_TODO(); }
    Fraction prob(const T& outcome) const { PRACTICE_TODO(); }

    Fraction expected() const
        requires std::convertible_to<T, Fraction>
    {
        PRACTICE_TODO();
    }

    template <typename F>
    Fraction expected_of(F f) const {
        PRACTICE_TODO();
    }

    template <typename F>
    Pmf<std::decay_t<std::invoke_result_t<F&, const T&>>> map(F f) const {
        PRACTICE_TODO();
    }
};

// Stage 3
template <typename A, typename B>
Pmf<std::pair<A, B>> independent(const Pmf<A>& a, const Pmf<B>& b) {
    PRACTICE_TODO();
}

// Stage 4
inline Pmf<int> convolve(const Pmf<int>& a, const Pmf<int>& b) { PRACTICE_TODO(); }

inline Fraction at_least_k_up(int replicas, const Fraction& p_up, int k) { PRACTICE_TODO(); }
