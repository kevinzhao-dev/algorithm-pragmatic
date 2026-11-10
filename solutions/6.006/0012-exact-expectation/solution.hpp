#pragma once

#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <map>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

class Fraction {
public:
    Fraction() = default;
    Fraction(std::int64_t numerator, std::int64_t denominator = 1) {
        if (denominator == 0) {
            throw std::invalid_argument("zero denominator");
        }
        assign(numerator, denominator);
    }

    std::int64_t numerator() const { return num_; }
    std::int64_t denominator() const { return den_; }

    friend Fraction operator+(const Fraction& a, const Fraction& b) {
        Fraction out;
        out.assign(Wide{a.num_} * b.den_ + Wide{b.num_} * a.den_, Wide{a.den_} * b.den_);
        return out;
    }
    friend Fraction operator-(const Fraction& a, const Fraction& b) {
        Fraction out;
        out.assign(Wide{a.num_} * b.den_ - Wide{b.num_} * a.den_, Wide{a.den_} * b.den_);
        return out;
    }
    friend Fraction operator*(const Fraction& a, const Fraction& b) {
        Fraction out;
        out.assign(Wide{a.num_} * b.num_, Wide{a.den_} * b.den_);
        return out;
    }
    friend Fraction operator/(const Fraction& a, const Fraction& b) {
        if (b.num_ == 0) {
            throw std::domain_error("division by zero");
        }
        Fraction out;
        out.assign(Wide{a.num_} * b.den_, Wide{a.den_} * b.num_);
        return out;
    }

    friend bool operator==(const Fraction& a, const Fraction& b) = default;
    friend std::strong_ordering operator<=>(const Fraction& a, const Fraction& b) {
        const Wide left = Wide{a.num_} * b.den_;
        const Wide right = Wide{b.num_} * a.den_;
        if (left < right) {
            return std::strong_ordering::less;
        }
        return left > right ? std::strong_ordering::greater : std::strong_ordering::equal;
    }

    friend std::ostream& operator<<(std::ostream& os, const Fraction& f) {
        return os << f.num_ << '/' << f.den_;
    }

private:
    using Wide = __int128;

    static Wide gcd(Wide a, Wide b) {
        while (b != 0) {
            a = std::exchange(b, a % b);
        }
        return a < 0 ? -a : a;
    }

    // Reduces num/den (den != 0) and stores it, or throws if it does not fit in 64 bits.
    void assign(Wide num, Wide den) {
        if (den < 0) {
            num = -num;
            den = -den;
        }
        const Wide divisor = gcd(num, den);
        num /= divisor;
        den /= divisor;
        constexpr Wide kMax = std::numeric_limits<std::int64_t>::max();
        constexpr Wide kMin = std::numeric_limits<std::int64_t>::min();
        if (num > kMax || num < kMin || den > kMax) {
            throw std::overflow_error("fraction does not fit in 64 bits");
        }
        num_ = static_cast<std::int64_t>(num);
        den_ = static_cast<std::int64_t>(den);
    }

    std::int64_t num_ = 0;
    std::int64_t den_ = 1;
};

template <typename T>
class Pmf {
public:
    explicit Pmf(const std::vector<std::pair<T, Fraction>>& outcomes) {
        Fraction total;
        for (const auto& [outcome, probability] : outcomes) {
            if (probability < Fraction{}) {
                throw std::invalid_argument("negative probability");
            }
            total = total + probability;
            if (probability != Fraction{}) {
                auto [it, inserted] = probabilities_.try_emplace(outcome, Fraction{});
                it->second = it->second + probability;
            }
        }
        if (total != Fraction{1}) {
            throw std::invalid_argument("probabilities do not sum to one");
        }
    }

    static Pmf uniform(const std::vector<T>& outcomes) {
        std::vector<std::pair<T, Fraction>> pairs;
        pairs.reserve(outcomes.size());
        const Fraction each(1, static_cast<std::int64_t>(outcomes.size()));
        for (const T& outcome : outcomes) {
            pairs.emplace_back(outcome, each);
        }
        return Pmf(pairs);
    }

    std::vector<T> support() const {
        std::vector<T> outcomes;
        outcomes.reserve(probabilities_.size());
        for (const auto& [outcome, probability] : probabilities_) {
            outcomes.push_back(outcome);
        }
        return outcomes;
    }

    Fraction prob(const T& outcome) const {
        const auto found = probabilities_.find(outcome);
        return found == probabilities_.end() ? Fraction{} : found->second;
    }

    Fraction expected() const
        requires std::convertible_to<T, Fraction>
    {
        return expected_of([](const T& outcome) { return Fraction(outcome); });
    }

    template <typename F>
    Fraction expected_of(F f) const {
        Fraction total;
        for (const auto& [outcome, probability] : probabilities_) {
            total = total + Fraction(f(outcome)) * probability;
        }
        return total;
    }

    template <typename F>
    Pmf<std::decay_t<std::invoke_result_t<F&, const T&>>> map(F f) const {
        using Image = std::decay_t<std::invoke_result_t<F&, const T&>>;
        std::vector<std::pair<Image, Fraction>> pairs;
        pairs.reserve(probabilities_.size());
        for (const auto& [outcome, probability] : probabilities_) {
            pairs.emplace_back(f(outcome), probability);
        }
        return Pmf<Image>(pairs);
    }

private:
    std::map<T, Fraction> probabilities_;
};

template <typename A, typename B>
Pmf<std::pair<A, B>> independent(const Pmf<A>& a, const Pmf<B>& b) {
    std::vector<std::pair<std::pair<A, B>, Fraction>> pairs;
    for (const A& x : a.support()) {
        for (const B& y : b.support()) {
            pairs.emplace_back(std::pair<A, B>{x, y}, a.prob(x) * b.prob(y));
        }
    }
    return Pmf<std::pair<A, B>>(pairs);
}

inline Pmf<int> convolve(const Pmf<int>& a, const Pmf<int>& b) {
    return independent(a, b).map([](const std::pair<int, int>& p) { return p.first + p.second; });
}

inline Fraction at_least_k_up(int replicas, const Fraction& p_up, int k) {
    if (k <= 0) {
        return Fraction(1);
    }
    if (k > replicas) {
        return Fraction(0);
    }
    const Pmf<int> single({{1, p_up}, {0, Fraction(1) - p_up}});
    Pmf<int> up_count({{0, Fraction(1)}});
    for (int i = 0; i < replicas; ++i) {
        up_count = convolve(up_count, single);
    }
    Fraction total;
    for (int count = k; count <= replicas; ++count) {
        total = total + up_count.prob(count);
    }
    return total;
}
