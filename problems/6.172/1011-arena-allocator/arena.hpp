// arena.hpp — Stage 1: a fixed anonymous mapping that only ever grows from the front.
#pragma once

#include <cstddef>

#include "practice.hpp"

class Arena {
public:
    explicit Arena(std::size_t capacity) { PRACTICE_TODO(); }
    ~Arena() {}

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Arena(Arena&& other) { PRACTICE_TODO(); }
    Arena& operator=(Arena&& other) { PRACTICE_TODO(); }

    std::byte* grow(std::size_t bytes) { PRACTICE_TODO(); }

    std::byte* base() const { PRACTICE_TODO(); }
    std::size_t used() const { PRACTICE_TODO(); }
    std::size_t capacity() const { PRACTICE_TODO(); }
};
