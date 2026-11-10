#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <complex>
#include <cstddef>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace wildcard_detail {

using Complex = std::complex<double>;

inline void fft(std::span<Complex> a, bool inverse) {
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

inline std::vector<std::size_t> every_position(std::size_t count) {
    std::vector<std::size_t> positions(count);
    for (std::size_t i = 0; i < count; ++i) {
        positions[i] = i;
    }
    return positions;
}

inline void validate(std::string_view text, std::string_view pattern, std::string_view alphabet) {
    const auto known = [&](char c) { return alphabet.find(c) != std::string_view::npos; };
    if (!std::ranges::all_of(text, known) ||
        !std::ranges::all_of(pattern, [&](char c) { return c == '*' || known(c); })) {
        throw std::invalid_argument("symbol outside the alphabet");
    }
}

}  // namespace wildcard_detail

inline std::vector<std::size_t> match_naive(std::string_view text, std::string_view pattern) {
    wildcard_detail::validate(text, pattern, "ab");
    std::vector<std::size_t> positions;
    if (pattern.size() > text.size()) {
        return positions;
    }
    for (std::size_t start = 0; start + pattern.size() <= text.size(); ++start) {
        bool matched = true;
        for (std::size_t i = 0; i < pattern.size() && matched; ++i) {
            matched = pattern[i] == '*' || pattern[i] == text[start + i];
        }
        if (matched) {
            positions.push_back(start);
        }
    }
    return positions;
}

inline std::vector<std::size_t> match_fft(std::string_view text, std::string_view pattern) {
    using wildcard_detail::Complex;
    wildcard_detail::validate(text, pattern, "ab");
    const std::size_t n = text.size();
    const std::size_t m = pattern.size();
    if (m > n) {
        return {};
    }
    if (m == 0) {
        return wildcard_detail::every_position(n + 1);
    }
    const std::size_t size = std::bit_ceil(n + m - 1);
    std::vector<Complex> s(size);
    std::vector<Complex> p(size);
    std::size_t required = 0;
    for (std::size_t i = 0; i < n; ++i) {
        s[i] = text[i] == 'a' ? 1.0 : -1.0;
    }
    for (std::size_t i = 0; i < m; ++i) {
        const char symbol = pattern[m - 1 - i];
        if (symbol == '*') {
            p[i] = 0.0;
        } else {
            p[i] = symbol == 'a' ? 1.0 : -1.0;
            ++required;
        }
    }
    wildcard_detail::fft(s, false);
    wildcard_detail::fft(p, false);
    for (std::size_t i = 0; i < size; ++i) {
        s[i] *= p[i];
    }
    wildcard_detail::fft(s, true);

    std::vector<std::size_t> positions;
    for (std::size_t start = 0; start + m <= n; ++start) {
        if (std::llround(s[start + m - 1].real()) == static_cast<long long>(required)) {
            positions.push_back(start);
        }
    }
    return positions;
}

inline std::vector<std::size_t> match_alphabet(std::string_view text, std::string_view pattern,
                                               std::string_view alphabet) {
    if (alphabet.empty() || alphabet.find('*') != std::string_view::npos) {
        throw std::invalid_argument("alphabet must be non-empty and exclude the wildcard");
    }
    std::string sorted(alphabet);
    std::ranges::sort(sorted);
    if (std::ranges::adjacent_find(sorted) != sorted.end()) {
        throw std::invalid_argument("alphabet symbols must be distinct");
    }
    wildcard_detail::validate(text, pattern, alphabet);

    const unsigned bits = std::max(1u, static_cast<unsigned>(std::bit_width(alphabet.size() - 1)));
    std::unordered_map<char, std::string> codes;
    for (std::size_t index = 0; index < alphabet.size(); ++index) {
        std::string code(bits, 'a');
        for (unsigned bit = 0; bit < bits; ++bit) {
            if ((index >> bit) & 1) {
                code[bit] = 'b';
            }
        }
        codes[alphabet[index]] = std::move(code);
    }

    std::string encoded_text;
    encoded_text.reserve(text.size() * bits);
    for (char symbol : text) {
        encoded_text += codes[symbol];
    }
    std::string encoded_pattern;
    encoded_pattern.reserve(pattern.size() * bits);
    for (char symbol : pattern) {
        if (symbol == '*') {
            encoded_pattern.append(bits, '*');
        } else {
            encoded_pattern += codes[symbol];
        }
    }

    std::vector<std::size_t> positions;
    for (std::size_t position : match_fft(encoded_text, encoded_pattern)) {
        if (position % bits == 0) {
            positions.push_back(position / bits);
        }
    }
    return positions;
}
