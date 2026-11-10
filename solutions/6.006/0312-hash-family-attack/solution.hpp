#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

struct Fraction {
    std::uint64_t num = 0;
    std::uint64_t den = 1;
    friend bool operator==(const Fraction&, const Fraction&) = default;
};

using KeyPair = std::pair<std::uint64_t, std::uint64_t>;

// (a k + b) mod n: two keys collide for every (a, b) exactly when they are congruent mod n.
inline std::optional<KeyPair> break_linear_family(std::uint64_t n, std::uint64_t u) {
    if (u < n + 2) {
        return std::nullopt;
    }
    return KeyPair{1, n + 1};
}

// (floor(k n / u) + a) mod n ignores everything but the block floor(k n / u).
inline std::optional<KeyPair> break_bucket_family(std::uint64_t n, std::uint64_t u) {
    const auto block = [&](std::uint64_t k) {
        return static_cast<std::uint64_t>(static_cast<unsigned __int128>(k) * n / u);
    };
    // Among the first n + 1 keys two must share a block, and blocks are monotone in k.
    for (std::uint64_t k = 1; k + 1 < u && k <= n; ++k) {
        if (block(k) == block(k + 1)) {
            return KeyPair{k, k + 1};
        }
    }
    return std::nullopt;
}

// Over the universal family, (a k1 + b, a k2 + b) mod p is a uniform ordered pair of distinct
// residues, so the collision probability does not depend on the keys at all.
inline Fraction universal_collision_probability(std::uint64_t p, std::uint64_t n) {
    const std::uint64_t q = p / n;
    const std::uint64_t t = p % n;
    const std::uint64_t colliding = t * (q + 1) * q + (n - t) * q * (q - 1);
    const std::uint64_t total = p * (p - 1);
    const std::uint64_t divisor = std::gcd(colliding, total);
    return Fraction{colliding / divisor, total / divisor};
}

inline std::vector<std::uint64_t> flood_linear(std::uint64_t n, std::uint64_t u, std::size_t count) {
    if (count == 0 || u < 2) {
        return {};
    }
    if (static_cast<unsigned __int128>(count - 1) * n + 1 >= u) {
        return {};
    }
    std::vector<std::uint64_t> keys;
    keys.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        keys.push_back(1 + i * n);
    }
    return keys;
}

inline std::vector<std::uint64_t> flood_bucket(std::uint64_t n, std::uint64_t u, std::size_t count) {
    if (count == 0 || u < 2) {
        return {};
    }
    const auto ceil_div = [&](std::uint64_t block) {
        const unsigned __int128 scaled = static_cast<unsigned __int128>(block) * u;
        return static_cast<std::uint64_t>((scaled + n - 1) / n);
    };
    // Block b holds the keys in [ceil(b u / n), ceil((b + 1) u / n)); key 0 is not allowed.
    for (std::uint64_t block = 0; block < n; ++block) {
        const std::uint64_t first = std::max<std::uint64_t>(1, ceil_div(block));
        const std::uint64_t past_last = ceil_div(block + 1);
        if (past_last > first && past_last - first >= count) {
            std::vector<std::uint64_t> keys;
            keys.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                keys.push_back(first + i);
            }
            return keys;
        }
    }
    return {};
}
