#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using Complex = std::complex<double>;

inline constexpr std::uint32_t kNttModulus = 998'244'353;

inline std::vector<std::int64_t> multiply_naive(std::span<const std::int64_t> a,
                                                std::span<const std::int64_t> b) {
    if (a.empty() || b.empty()) {
        return {};
    }
    std::vector<std::int64_t> product(a.size() + b.size() - 1, 0);
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (std::size_t j = 0; j < b.size(); ++j) {
            product[i + j] += a[i] * b[j];
        }
    }
    return product;
}

inline void fft(std::span<Complex> a, bool inverse = false) {
    const std::size_t n = a.size();
    if (!std::has_single_bit(n)) {
        throw std::invalid_argument("fft: size must be a power of two");
    }

    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(a[i], a[j]);
        }
    }

    // Each root is computed directly. Accumulating w *= step would add O(n * epsilon) error,
    // which is what breaks integer rounding at this contract's size limit.
    const double sign = inverse ? -1.0 : 1.0;
    const double step = sign * 2.0 * std::numbers::pi / static_cast<double>(n);
    std::vector<Complex> roots(n / 2);
    for (std::size_t k = 0; k < n / 2; ++k) {
        const double angle = step * static_cast<double>(k);
        roots[k] = Complex(std::cos(angle), std::sin(angle));
    }

    for (std::size_t length = 2; length <= n; length <<= 1) {
        const std::size_t half = length / 2;
        const std::size_t stride = n / length;
        for (std::size_t start = 0; start < n; start += length) {
            for (std::size_t k = 0; k < half; ++k) {
                const Complex u = a[start + k];
                const Complex v = a[start + k + half] * roots[k * stride];
                a[start + k] = u + v;
                a[start + k + half] = u - v;
            }
        }
    }

    if (inverse) {
        for (Complex& value : a) {
            value /= static_cast<double>(n);
        }
    }
}

namespace poly_detail {

inline constexpr double kExactProductLimit = 4398046511104.0;  // 2^42

inline double largest_magnitude(std::span<const std::int64_t> values) {
    double largest = 0.0;
    for (std::int64_t value : values) {
        largest = std::max(largest, std::abs(static_cast<double>(value)));
    }
    return largest;
}

}  // namespace poly_detail

inline std::vector<std::int64_t> multiply(std::span<const std::int64_t> a,
                                          std::span<const std::int64_t> b) {
    if (a.empty() || b.empty()) {
        return {};
    }
    const double scale = static_cast<double>(a.size() + b.size()) *
                         poly_detail::largest_magnitude(a) * poly_detail::largest_magnitude(b);
    if (scale > poly_detail::kExactProductLimit) {
        throw std::domain_error("multiply: input exceeds the exact-rounding contract");
    }

    const std::size_t length = a.size() + b.size() - 1;
    const std::size_t n = std::bit_ceil(length);
    std::vector<Complex> left(n);
    std::vector<Complex> right(n);
    for (std::size_t i = 0; i < a.size(); ++i) {
        left[i] = static_cast<double>(a[i]);
    }
    for (std::size_t i = 0; i < b.size(); ++i) {
        right[i] = static_cast<double>(b[i]);
    }
    fft(left);
    fft(right);
    for (std::size_t i = 0; i < n; ++i) {
        left[i] *= right[i];
    }
    fft(left, true);

    std::vector<std::int64_t> product(length);
    for (std::size_t i = 0; i < length; ++i) {
        product[i] = std::llround(left[i].real());
    }
    return product;
}

namespace poly_detail {

inline std::uint32_t mod_mul(std::uint32_t a, std::uint32_t b) {
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(a) * b % kNttModulus);
}

inline std::uint32_t mod_pow(std::uint32_t base, std::uint64_t exponent) {
    std::uint32_t result = 1;
    while (exponent != 0) {
        if (exponent & 1) {
            result = mod_mul(result, base);
        }
        base = mod_mul(base, base);
        exponent >>= 1;
    }
    return result;
}

inline void ntt(std::span<std::uint32_t> a, bool inverse) {
    const std::size_t n = a.size();
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(a[i], a[j]);
        }
    }

    const std::uint32_t generator = 3;
    for (std::size_t length = 2; length <= n; length <<= 1) {
        std::uint32_t root = mod_pow(generator, (kNttModulus - 1) / length);
        if (inverse) {
            root = mod_pow(root, kNttModulus - 2);
        }
        const std::size_t half = length / 2;
        std::vector<std::uint32_t> powers(half, 1);
        for (std::size_t k = 1; k < half; ++k) {
            powers[k] = mod_mul(powers[k - 1], root);
        }
        for (std::size_t start = 0; start < n; start += length) {
            for (std::size_t k = 0; k < half; ++k) {
                const std::uint32_t u = a[start + k];
                const std::uint32_t v = mod_mul(a[start + k + half], powers[k]);
                a[start + k] = u + v >= kNttModulus ? u + v - kNttModulus : u + v;
                a[start + k + half] = u >= v ? u - v : u + kNttModulus - v;
            }
        }
    }

    if (inverse) {
        const std::uint32_t scale = mod_pow(static_cast<std::uint32_t>(n), kNttModulus - 2);
        for (std::uint32_t& value : a) {
            value = mod_mul(value, scale);
        }
    }
}

}  // namespace poly_detail

inline std::vector<std::uint32_t> multiply_mod(std::span<const std::uint32_t> a,
                                               std::span<const std::uint32_t> b) {
    if (a.empty() || b.empty()) {
        return {};
    }
    const std::size_t length = a.size() + b.size() - 1;
    if (length > (std::size_t{1} << 23)) {
        throw std::length_error("multiply_mod: product is longer than the modulus supports");
    }
    for (std::span<const std::uint32_t> side : {a, b}) {
        for (std::uint32_t value : side) {
            if (value >= kNttModulus) {
                throw std::invalid_argument("multiply_mod: coefficient is not reduced");
            }
        }
    }

    const std::size_t n = std::bit_ceil(length);
    std::vector<std::uint32_t> left(a.begin(), a.end());
    std::vector<std::uint32_t> right(b.begin(), b.end());
    left.resize(n);
    right.resize(n);
    poly_detail::ntt(left, false);
    poly_detail::ntt(right, false);
    for (std::size_t i = 0; i < n; ++i) {
        left[i] = poly_detail::mod_mul(left[i], right[i]);
    }
    poly_detail::ntt(left, true);
    left.resize(length);
    return left;
}

inline std::string multiply_decimal(std::string_view a, std::string_view b) {
    constexpr std::size_t kChunkDigits = 3;  // Base 1000 stays inside multiply()'s contract.
    constexpr std::int64_t kBase = 1000;

    const auto parse = [](std::string_view text, bool& negative) {
        negative = !text.empty() && text.front() == '-';
        if (negative) {
            text.remove_prefix(1);
        }
        if (text.empty() || !std::ranges::all_of(text, [](char c) { return '0' <= c && c <= '9'; })) {
            throw std::invalid_argument("multiply_decimal: malformed number");
        }
        const auto first_digit = text.find_first_not_of('0');
        text.remove_prefix(first_digit == std::string_view::npos ? text.size() - 1 : first_digit);
        std::vector<std::int64_t> limbs;
        limbs.reserve(text.size() / kChunkDigits + 1);
        for (std::size_t end = text.size(); end != 0;) {
            const std::size_t begin = end > kChunkDigits ? end - kChunkDigits : 0;
            std::int64_t limb = 0;
            for (std::size_t i = begin; i < end; ++i) {
                limb = limb * 10 + (text[i] - '0');
            }
            limbs.push_back(limb);
            end = begin;
        }
        return limbs;
    };

    bool a_negative = false;
    bool b_negative = false;
    const auto left = parse(a, a_negative);
    const auto right = parse(b, b_negative);

    std::vector<std::int64_t> limbs = multiply(left, right);
    limbs.push_back(0);
    for (std::size_t i = 0; i + 1 < limbs.size(); ++i) {
        limbs[i + 1] += limbs[i] / kBase;
        limbs[i] %= kBase;
    }
    while (limbs.size() > 1 && limbs.back() == 0) {
        limbs.pop_back();
    }

    std::string out = std::to_string(limbs.back());
    for (std::size_t i = limbs.size() - 1; i-- > 0;) {
        out += std::string(kChunkDigits - std::to_string(limbs[i]).size(), '0');
        out += std::to_string(limbs[i]);
    }
    if (a_negative != b_negative && out != "0") {
        out.insert(out.begin(), '-');
    }
    return out;
}
