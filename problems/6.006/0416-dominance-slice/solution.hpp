#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

#include "practice.hpp"

class PrefixTree {
public:
    // Stage 1
    PrefixTree() { PRACTICE_TODO(); }
    bool insert(std::int64_t key, std::int64_t value) { PRACTICE_TODO(); }
    bool erase(std::int64_t key) { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    std::optional<std::int64_t> value_of(std::int64_t key) const { PRACTICE_TODO(); }
    std::optional<std::pair<std::int64_t, std::int64_t>> max_prefix() const { PRACTICE_TODO(); }

    // Stage 3
    void add(std::int64_t key, std::int64_t delta) { PRACTICE_TODO(); }
};

struct Topping {
    std::int64_t x;
    std::int64_t y;
    std::int64_t t;
};

struct Slice {
    std::int64_t x;
    std::int64_t y;
    std::int64_t t;
    friend bool operator==(const Slice&, const Slice&) = default;
};

// Stage 2 (and stage 3)
inline std::optional<Slice> tastiest_slice(std::span<const Topping> toppings) { PRACTICE_TODO(); }
