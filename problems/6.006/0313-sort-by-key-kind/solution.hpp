#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

#include "practice.hpp"

// Move-only on purpose: rearrange slices by moving them, never by copying.
struct Slice {
    Slice(std::string id, std::uint32_t age_years, std::uint64_t thickness, std::uint64_t memory_value,
          int tag_value)
        : core_id(std::move(id)),
          age(age_years),
          thickness_numerator(thickness),
          memory(memory_value),
          tag(tag_value) {}
    Slice(Slice&&) = default;
    Slice& operator=(Slice&&) = default;

    std::string core_id;
    std::uint32_t age;
    std::uint64_t thickness_numerator;
    std::uint64_t memory;
    int tag;
};

// Stage 1
inline void sort_by_core_id(std::span<Slice> slices) { PRACTICE_TODO(); }

// Stage 2
inline void sort_by_age(std::span<Slice> slices) { PRACTICE_TODO(); }

// Stage 3
inline void sort_by_thickness(std::span<Slice> slices) { PRACTICE_TODO(); }

// Stage 4
template <typename Less>
void sort_opaque(std::span<Slice> slices, Less less) {
    PRACTICE_TODO();
}
