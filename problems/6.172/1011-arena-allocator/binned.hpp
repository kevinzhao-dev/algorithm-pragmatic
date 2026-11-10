// binned.hpp — Stage 3.
#pragma once

#include <cstddef>

#include "arena.hpp"
#include "practice.hpp"

class BinnedAllocator {
public:
    explicit BinnedAllocator(std::size_t capacity) : arena_(capacity) {}

    void* malloc(std::size_t size) { PRACTICE_TODO(); }
    void free(void* ptr) { PRACTICE_TODO(); }
    void* realloc(void* ptr, std::size_t size) { PRACTICE_TODO(); }
    bool check() const { PRACTICE_TODO(); }

    std::size_t heap_bytes() const { return arena_.used(); }
    const Arena& arena() const { return arena_; }

private:
    Arena arena_;
};
