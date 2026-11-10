#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

#include "practice.hpp"

inline constexpr std::uint64_t kHashPrime = (std::uint64_t{1} << 61) - 1;

// Stage 1
class DotProductHash {
public:
    DotProductHash(std::uint64_t slots, std::mt19937_64& rng) { PRACTICE_TODO(); }

    std::uint64_t slots() const { PRACTICE_TODO(); }
    std::uint64_t operator()(std::uint64_t key) const { PRACTICE_TODO(); }
};

// Stage 2
class LinearHash {
public:
    LinearHash(std::uint64_t slots, std::mt19937_64& rng) { PRACTICE_TODO(); }

    std::uint64_t slots() const { PRACTICE_TODO(); }
    std::uint64_t operator()(std::uint64_t key) const { PRACTICE_TODO(); }
};

// Stage 3
template <typename V>
class UniversalHashMap {
public:
    explicit UniversalHashMap(std::uint64_t seed = 6046) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    std::size_t bucket_count() const { PRACTICE_TODO(); }
    std::size_t longest_chain() const { PRACTICE_TODO(); }

    bool insert(std::uint64_t key, V value) { PRACTICE_TODO(); }
    V* find(std::uint64_t key) { PRACTICE_TODO(); }
    const V* find(std::uint64_t key) const { PRACTICE_TODO(); }
    bool erase(std::uint64_t key) { PRACTICE_TODO(); }
};

// Stage 4
class RateLimiter {
public:
    RateLimiter(std::uint64_t limit, std::int64_t window, std::uint64_t seed = 6046) { PRACTICE_TODO(); }

    bool allow(std::uint64_t client, std::int64_t now) { PRACTICE_TODO(); }
    std::size_t client_count() const { PRACTICE_TODO(); }
    std::size_t longest_chain() const { PRACTICE_TODO(); }
};
