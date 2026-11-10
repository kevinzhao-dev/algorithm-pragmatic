#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

#include "../0303-counting-radix-sort/solution.hpp"

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

inline void sort_by_core_id(std::span<Slice> slices) {
    if (slices.empty()) {
        return;
    }
    const std::size_t length = slices.front().core_id.size();
    tuple_sort(slices, length, 128, [length](const Slice& slice, std::size_t column) {
        return static_cast<std::size_t>(slice.core_id[length - 1 - column]);
    });
}

inline void sort_by_age(std::span<Slice> slices) {
    radix_sort(slices, [](const Slice& slice) { return slice.age; });
}

inline void sort_by_thickness(std::span<Slice> slices) {
    radix_sort(slices, [](const Slice& slice) { return slice.thickness_numerator; });
}

template <typename Less>
void sort_opaque(std::span<Slice> slices, Less less) {
    std::stable_sort(slices.begin(), slices.end(), less);
}
