// Fixed baselines for the merge-sort tuning stages. Same algorithmic shape as the routines you
// write, deliberately without the optimization each stage asks for.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <span>

namespace baseline {

using ErasedCompare = int (*)(const void*, const void*);

struct KeyedRecord {
    std::uint64_t key;
    std::uint64_t payload;
};

[[gnu::noinline]] inline int compare_records(const void* a, const void* b) {
    const std::uint64_t x = static_cast<const KeyedRecord*>(a)->key;
    const std::uint64_t y = static_cast<const KeyedRecord*>(b)->key;
    return (x > y) - (x < y);
}

// qsort-style: every comparison is an indirect call and every element move is a memcpy.
[[gnu::noinline]] inline void merge_erased(char* data, std::size_t begin, std::size_t mid,
                                           std::size_t end, std::size_t size,
                                           ErasedCompare compare) {
    const std::size_t left_count = mid - begin;
    const std::size_t right_count = end - mid;
    auto* left = static_cast<char*>(std::malloc(left_count * size));
    auto* right = static_cast<char*>(std::malloc(right_count * size));
    std::memcpy(left, data + begin * size, left_count * size);
    std::memcpy(right, data + mid * size, right_count * size);
    std::size_t i = 0;
    std::size_t j = 0;
    char* out = data + begin * size;
    while (i < left_count && j < right_count) {
        if (compare(right + j * size, left + i * size) < 0) {
            std::memcpy(out, right + j++ * size, size);
        } else {
            std::memcpy(out, left + i++ * size, size);
        }
        out += size;
    }
    while (i < left_count) {
        std::memcpy(out, left + i++ * size, size);
        out += size;
    }
    while (j < right_count) {
        std::memcpy(out, right + j++ * size, size);
        out += size;
    }
    std::free(left);
    std::free(right);
}

[[gnu::noinline]] inline void erased_sort(char* data, std::size_t begin, std::size_t end,
                                          std::size_t size, ErasedCompare compare) {
    if (end - begin < 2) {
        return;
    }
    const std::size_t mid = begin + (end - begin) / 2;
    erased_sort(data, begin, mid, size, compare);
    erased_sort(data, mid, end, size, compare);
    merge_erased(data, begin, mid, end, size, compare);
}

// A typed baseline: recursion all the way to one element, and two fresh arrays per merge.
[[gnu::noinline]] inline bool less_u32(std::uint32_t a, std::uint32_t b) {
    return a < b;
}

[[gnu::noinline]] inline void merge_two_arrays(std::uint32_t* data, std::size_t begin,
                                               std::size_t mid, std::size_t end,
                                               bool (*less)(std::uint32_t, std::uint32_t)) {
    const std::size_t left_count = mid - begin;
    const std::size_t right_count = end - mid;
    auto* left = static_cast<std::uint32_t*>(std::malloc(left_count * sizeof(std::uint32_t)));
    auto* right = static_cast<std::uint32_t*>(std::malloc(right_count * sizeof(std::uint32_t)));
    std::memcpy(left, data + begin, left_count * sizeof(std::uint32_t));
    std::memcpy(right, data + mid, right_count * sizeof(std::uint32_t));
    std::size_t i = 0;
    std::size_t j = 0;
    std::size_t k = begin;
    while (i < left_count && j < right_count) {
        data[k++] = less(right[j], left[i]) ? right[j++] : left[i++];
    }
    while (i < left_count) {
        data[k++] = left[i++];
    }
    while (j < right_count) {
        data[k++] = right[j++];
    }
    std::free(left);
    std::free(right);
}

[[gnu::noinline]] inline void fine_grained_sort(std::uint32_t* data, std::size_t begin,
                                                std::size_t end,
                                                bool (*less)(std::uint32_t, std::uint32_t)) {
    if (end - begin < 2) {
        return;
    }
    const std::size_t mid = begin + (end - begin) / 2;
    fine_grained_sort(data, begin, mid, less);
    fine_grained_sort(data, mid, end, less);
    merge_two_arrays(data, begin, mid, end, less);
}

// Fields of `view` are reloaded on every iteration: a write through uint8_t* may alias them.
struct ByteView {
    std::uint8_t* data;
    std::size_t size;
};

[[gnu::noinline]] inline void add_with_reload(ByteView& view, std::uint8_t delta) {
    for (std::size_t i = 0; i < view.size; ++i) {
        view.data[i] = static_cast<std::uint8_t>(view.data[i] + delta);
    }
}

}  // namespace baseline
