#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "practice.hpp"

// Stage 1
class UniversalHash {
public:
    static constexpr std::uint64_t kPrime = (std::uint64_t{1} << 61) - 1;

    UniversalHash(std::uint64_t a, std::uint64_t b) { PRACTICE_TODO(); }
    std::size_t operator()(std::uint64_t key, std::size_t buckets) const { PRACTICE_TODO(); }
};

template <typename K, typename V, typename Prehash = std::hash<K>>
class ChainedHashMap {
public:
    // Stage 2
    explicit ChainedHashMap(std::uint64_t seed = 0x9E3779B97F4A7C15ULL, Prehash prehash = Prehash{}) {
        PRACTICE_TODO();
    }

    ChainedHashMap(const ChainedHashMap&) = delete;
    ChainedHashMap& operator=(const ChainedHashMap&) = delete;

    // Stage 4. Once implemented, these should be noexcept.
    ChainedHashMap(ChainedHashMap&& other) { PRACTICE_TODO(); }
    ChainedHashMap& operator=(ChainedHashMap&& other) { PRACTICE_TODO(); }

    // Stage 2
    bool insert(K key, V value) { PRACTICE_TODO(); }
    V* find(const K& key) { PRACTICE_TODO(); }
    const V* find(const K& key) const { PRACTICE_TODO(); }
    std::optional<V> erase(const K& key) { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }

    // Stage 3
    std::size_t bucket_count() const { PRACTICE_TODO(); }
    double load_factor() const { PRACTICE_TODO(); }

    // Stage 4
    std::size_t bucket_of(const K& key) const { PRACTICE_TODO(); }
    std::size_t max_chain_length() const { PRACTICE_TODO(); }
};
