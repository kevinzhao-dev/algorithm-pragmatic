// 06_fraction.hpp — exact rational arithmetic for the linear-programming exercises.
//
// Frac keeps a reduced numerator/denominator in 128 bits with a positive denominator, so the LP
// solvers never need a tolerance: comparisons are exact. Any operation that would overflow
// throws std::overflow_error instead of returning garbage.
#pragma once

#include <compare>
#include <ostream>
#include <stdexcept>
#include <string>

class Frac {
public:
    constexpr Frac() = default;
    constexpr Frac(long long value) : numerator_(value), denominator_(1) {}  // NOLINT: implicit on purpose
    Frac(long long numerator, long long denominator)
        : numerator_(numerator), denominator_(denominator) {
        if (denominator == 0) {
            throw std::invalid_argument("zero denominator");
        }
        normalize();
    }

    __int128 numerator() const { return numerator_; }
    __int128 denominator() const { return denominator_; }
    bool is_zero() const { return numerator_ == 0; }

    double to_double() const {
        return static_cast<double>(numerator_) / static_cast<double>(denominator_);
    }

    Frac operator-() const {
        Frac out = *this;
        out.numerator_ = -out.numerator_;
        return out;
    }

    friend Frac operator+(const Frac& a, const Frac& b) {
        const __int128 g = gcd(a.denominator_, b.denominator_);
        const __int128 ad = checked_mul(a.numerator_, b.denominator_ / g);
        const __int128 bd = checked_mul(b.numerator_, a.denominator_ / g);
        Frac out;
        out.numerator_ = checked_add(ad, bd);
        out.denominator_ = checked_mul(a.denominator_ / g, b.denominator_);
        out.normalize();
        return out;
    }
    friend Frac operator-(const Frac& a, const Frac& b) { return a + (-b); }
    friend Frac operator*(const Frac& a, const Frac& b) {
        const __int128 g1 = gcd(abs(a.numerator_), b.denominator_);
        const __int128 g2 = gcd(abs(b.numerator_), a.denominator_);
        Frac out;
        out.numerator_ = checked_mul(a.numerator_ / g1, b.numerator_ / g2);
        out.denominator_ = checked_mul(a.denominator_ / g2, b.denominator_ / g1);
        out.normalize();
        return out;
    }
    friend Frac operator/(const Frac& a, const Frac& b) {
        if (b.numerator_ == 0) {
            throw std::domain_error("division by zero");
        }
        Frac inverse;
        inverse.numerator_ = b.numerator_ < 0 ? -b.denominator_ : b.denominator_;
        inverse.denominator_ = b.numerator_ < 0 ? -b.numerator_ : b.numerator_;
        return a * inverse;
    }

    Frac& operator+=(const Frac& other) { return *this = *this + other; }
    Frac& operator-=(const Frac& other) { return *this = *this - other; }
    Frac& operator*=(const Frac& other) { return *this = *this * other; }
    Frac& operator/=(const Frac& other) { return *this = *this / other; }

    friend bool operator==(const Frac& a, const Frac& b) {
        return a.numerator_ == b.numerator_ && a.denominator_ == b.denominator_;
    }
    friend std::strong_ordering operator<=>(const Frac& a, const Frac& b) {
        const __int128 left = checked_mul(a.numerator_, b.denominator_);
        const __int128 right = checked_mul(b.numerator_, a.denominator_);
        return left < right ? std::strong_ordering::less
                            : (left > right ? std::strong_ordering::greater
                                            : std::strong_ordering::equal);
    }

    friend std::ostream& operator<<(std::ostream& os, const Frac& f) {
        os << to_text(f.numerator_);
        if (f.denominator_ != 1) {
            os << '/' << to_text(f.denominator_);
        }
        return os;
    }

private:
    static __int128 abs(__int128 v) { return v < 0 ? -v : v; }

    static __int128 gcd(__int128 a, __int128 b) {
        while (b != 0) {
            const __int128 t = a % b;
            a = b;
            b = t;
        }
        return a;
    }

    static __int128 checked_mul(__int128 a, __int128 b) {
        __int128 out = 0;
        if (__builtin_mul_overflow(a, b, &out)) {
            throw std::overflow_error("Frac: 128-bit overflow in multiplication");
        }
        return out;
    }

    static __int128 checked_add(__int128 a, __int128 b) {
        __int128 out = 0;
        if (__builtin_add_overflow(a, b, &out)) {
            throw std::overflow_error("Frac: 128-bit overflow in addition");
        }
        return out;
    }

    static std::string to_text(__int128 value) {
        if (value == 0) {
            return "0";
        }
        const bool negative = value < 0;
        std::string digits;
        while (value != 0) {
            int d = static_cast<int>(value % 10);
            digits.push_back(static_cast<char>('0' + (d < 0 ? -d : d)));
            value /= 10;
        }
        if (negative) {
            digits.push_back('-');
        }
        return {digits.rbegin(), digits.rend()};
    }

    void normalize() {
        if (denominator_ < 0) {
            numerator_ = -numerator_;
            denominator_ = -denominator_;
        }
        const __int128 g = gcd(abs(numerator_), denominator_);
        numerator_ /= g;
        denominator_ /= g;
    }

    __int128 numerator_ = 0;
    __int128 denominator_ = 1;
};
