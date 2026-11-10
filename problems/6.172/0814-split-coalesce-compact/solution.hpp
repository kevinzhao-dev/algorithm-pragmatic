#pragma once

#include <cstddef>
#include <optional>
#include <span>

#include "practice.hpp"

enum class Coalesce { Never, Adjacent };

// Stages 1 and 2: offsets only; all lines start out free.
class TwoSizeHeap {
public:
    TwoSizeHeap(std::size_t lines, Coalesce policy) { PRACTICE_TODO(); }

    std::optional<std::size_t> alloc_small() { PRACTICE_TODO(); }
    std::optional<std::size_t> alloc_large() { PRACTICE_TODO(); }
    void free_small(std::size_t offset) { PRACTICE_TODO(); }
    void free_large(std::size_t offset) { PRACTICE_TODO(); }
    std::size_t free_bytes() const { PRACTICE_TODO(); }
};

// Stage 3: objects live in real memory and can be moved; callers hold handles.
class CompactingHeap {
public:
    using Handle = std::size_t;

    explicit CompactingHeap(std::size_t lines) { PRACTICE_TODO(); }

    std::optional<Handle> alloc_small() { PRACTICE_TODO(); }
    std::optional<Handle> alloc_large() { PRACTICE_TODO(); }
    void free(Handle handle) { PRACTICE_TODO(); }
    std::span<std::byte> deref(Handle handle) { PRACTICE_TODO(); }
    void compact() { PRACTICE_TODO(); }
    std::size_t free_bytes() const { PRACTICE_TODO(); }
};
