#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

// Stage 1
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class Memo {
public:
    // relation is called as relation(key, recurse); recurse(k) returns const Value&.
    template <typename Relation>
    const Value& get(const Key& key, Relation&& relation) {
        PRACTICE_TODO();
    }

    std::size_t size() const { PRACTICE_TODO(); }
};

// Stage 2
inline std::int64_t bowling_max_score(std::span<const int> pins) { PRACTICE_TODO(); }

// Stage 3
struct Throw {
    std::size_t first;
    std::size_t last;  // first, or first + 1
};

inline std::vector<Throw> bowling_plan(std::span<const int> pins) { PRACTICE_TODO(); }

// Stage 4
class BowlingStream {
public:
    void push(int pin) { PRACTICE_TODO(); }
    std::int64_t best() const { PRACTICE_TODO(); }
};
