#pragma once

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <tuple>

inline void require_positive_modulus(std::int64_t m) {
    if (m <= 0) {
        throw std::invalid_argument("modulus must be positive");
    }
}

inline std::int64_t floor_mod(std::int64_t a, std::int64_t m) {
    require_positive_modulus(m);
    const std::int64_t remainder = a % m;
    return remainder < 0 ? remainder + m : remainder;
}

inline std::int64_t floor_div(std::int64_t a, std::int64_t m) {
    require_positive_modulus(m);
    const std::int64_t quotient = a / m;
    return (a % m != 0 && a < 0) ? quotient - 1 : quotient;
}

inline bool congruent(std::int64_t a, std::int64_t b, std::int64_t m) {
    return floor_mod(a, m) == floor_mod(b, m);
}

inline std::uint64_t mul_mod(std::uint64_t a, std::uint64_t b, std::uint64_t m) {
    return static_cast<std::uint64_t>(static_cast<unsigned __int128>(a) * b % m);
}

inline std::uint64_t pow_mod(std::uint64_t base, std::uint64_t exponent, std::uint64_t m) {
    std::uint64_t result = 1 % m;
    base %= m;
    while (exponent > 0) {
        if (exponent & 1) {
            result = mul_mod(result, base, m);
        }
        base = mul_mod(base, base, m);
        exponent >>= 1;
    }
    return result;
}

struct PeriodicJob {
    std::int64_t phase;
    std::int64_t period;
};

inline std::optional<std::int64_t> first_coincidence(PeriodicJob a, PeriodicJob b) {
    using Wide = __int128;

    // Solve a.phase + a.period * k ≡ b.phase (mod b.period) for the smallest k ≥ 0.
    const std::int64_t g = std::gcd(a.period, b.period);
    const std::int64_t difference = b.phase - a.phase;
    if (difference % g != 0) {
        return std::nullopt;
    }
    const std::int64_t reduced_p = a.period / g;
    const std::int64_t reduced_q = b.period / g;

    // Extended Euclid on (reduced_p mod reduced_q, reduced_q) gives reduced_p's inverse.
    std::int64_t old_r = floor_mod(reduced_p, reduced_q);
    std::int64_t r = reduced_q;
    std::int64_t old_s = 1;
    std::int64_t s = 0;
    while (r != 0) {
        const std::int64_t q = old_r / r;
        std::tie(old_r, r) = std::pair(r, old_r - q * r);
        std::tie(old_s, s) = std::pair(s, old_s - q * s);
    }
    const std::int64_t inverse = floor_mod(old_s, reduced_q);

    const auto k = static_cast<std::int64_t>(
        static_cast<Wide>(floor_mod(difference / g, reduced_q)) * inverse % reduced_q);
    const Wide lcm = static_cast<Wide>(a.period) * reduced_q;
    Wide t = static_cast<Wide>(a.phase) + static_cast<Wide>(a.period) * k;
    const Wide earliest = std::max(a.phase, b.phase);
    if (t < earliest) {
        t += (earliest - t + lcm - 1) / lcm * lcm;
    }
    return static_cast<std::int64_t>(t);
}
