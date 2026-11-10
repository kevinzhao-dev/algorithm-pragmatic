#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

#include "practice.hpp"

template <typename K, typename Hash = std::hash<K>>
class OpenAddressingSet {
public:
    // Stage 1
    explicit OpenAddressingSet(Hash hash = Hash{}) { PRACTICE_TODO(); }

    bool insert(K key) { PRACTICE_TODO(); }
    bool contains(const K& key) const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    std::size_t capacity() const { PRACTICE_TODO(); }

    // Stage 2
    bool erase(const K& key) { PRACTICE_TODO(); }
};

// Stage 4
template <typename K, typename Hash = std::hash<K>>
class DedupWindow {
public:
    explicit DedupWindow(std::int64_t ttl, Hash hash = Hash{}) { PRACTICE_TODO(); }

    bool offer(const K& key, std::int64_t now) { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
};
