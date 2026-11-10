#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace detail_0211 {

// Stage 1: straight translation with two temporary arrays per merge.
template <typename T, typename Compare>
void merge_a(std::span<T> data, std::size_t mid, Compare& cmp) {
    std::vector<T> left(data.begin(), data.begin() + static_cast<std::ptrdiff_t>(mid));
    std::vector<T> right(data.begin() + static_cast<std::ptrdiff_t>(mid), data.end());
    std::size_t i = 0;
    std::size_t j = 0;
    std::size_t k = 0;
    while (i < left.size() && j < right.size()) {
        if (cmp(right[j], left[i])) {
            data[k++] = right[j++];
        } else {
            data[k++] = left[i++];
        }
    }
    while (i < left.size()) {
        data[k++] = left[i++];
    }
    while (j < right.size()) {
        data[k++] = right[j++];
    }
}

}  // namespace detail_0211

template <typename T, typename Compare = std::less<>>
void sort_a(std::span<T> data, Compare cmp = {}) {
    if (data.size() < 2) {
        return;
    }
    const std::size_t mid = data.size() / 2;
    sort_a(data.first(mid), cmp);
    sort_a(data.subspan(mid), cmp);
    detail_0211::merge_a(data, mid, cmp);
}

// Stage 2: the comparator is a template parameter, so it inlines into the merge loop.
template <typename T, typename Compare = std::less<>>
void sort_i(std::span<T> data, Compare cmp = {}) {
    if (data.size() < 2) {
        return;
    }
    const std::size_t mid = data.size() / 2;
    sort_i(data.first(mid), cmp);
    sort_i(data.subspan(mid), cmp);
    detail_0211::merge_a(data, mid, cmp);
}

namespace detail_0211 {

template <typename T, typename Compare>
void merge_p(T* first, T* middle, T* last, Compare& cmp) {
    std::vector<T> left(first, middle);
    std::vector<T> right(middle, last);
    T* l = left.data();
    T* const l_end = l + left.size();
    T* r = right.data();
    T* const r_end = r + right.size();
    T* out = first;
    while (l != l_end && r != r_end) {
        *out++ = cmp(*r, *l) ? *r++ : *l++;
    }
    while (l != l_end) {
        *out++ = *l++;
    }
    while (r != r_end) {
        *out++ = *r++;
    }
}

}  // namespace detail_0211

// Stage 3
template <typename T, typename Compare = std::less<>>
void sort_p(std::span<T> data, Compare cmp = {}) {
    if (data.size() < 2) {
        return;
    }
    const std::size_t mid = data.size() / 2;
    sort_p(data.first(mid), cmp);
    sort_p(data.subspan(mid), cmp);
    detail_0211::merge_p(data.data(), data.data() + mid, data.data() + data.size(), cmp);
}

struct ByteView {
    std::uint8_t* data;
    std::size_t size;
};

[[gnu::noinline]] inline void add_to_all(ByteView& view, std::uint8_t delta) {
    // Copies of the fields cannot alias the bytes being written, so the loop vectorizes.
    std::uint8_t* const bytes = view.data;
    const std::size_t count = view.size;
    for (std::size_t i = 0; i < count; ++i) {
        bytes[i] = static_cast<std::uint8_t>(bytes[i] + delta);
    }
}

namespace detail_0211 {

template <typename T, typename Compare>
void insertion_sort(T* first, T* last, Compare& cmp) {
    for (T* current = first + 1; current < last; ++current) {
        T value = std::move(*current);
        T* hole = current;
        while (hole > first && cmp(value, *(hole - 1))) {
            *hole = std::move(*(hole - 1));
            --hole;
        }
        *hole = std::move(value);
    }
}

}  // namespace detail_0211

// Stage 4
template <typename T, typename Compare = std::less<>>
void sort_c(std::span<T> data, Compare cmp = {}, std::size_t cutoff = 16) {
    if (data.size() < 2) {
        return;
    }
    if (data.size() <= cutoff) {
        detail_0211::insertion_sort(data.data(), data.data() + data.size(), cmp);
        return;
    }
    const std::size_t mid = data.size() / 2;
    sort_c(data.first(mid), cmp, cutoff);
    sort_c(data.subspan(mid), cmp, cutoff);
    detail_0211::merge_p(data.data(), data.data() + mid, data.data() + data.size(), cmp);
}

namespace detail_0211 {

// Copies only the left half out; the right half stays where it is. The write cursor can never
// overtake the unread part of the right half, so nothing live gets overwritten.
template <typename T, typename Compare>
void merge_with_scratch(T* first, T* middle, T* last, T* scratch, Compare& cmp) {
    T* const scratch_end = std::move(first, middle, scratch);
    T* l = scratch;
    T* r = middle;
    T* out = first;
    while (l != scratch_end && r != last) {
        if (cmp(*r, *l)) {
            *out++ = std::move(*r++);
        } else {
            *out++ = std::move(*l++);
        }
    }
    std::move(l, scratch_end, out);
}

}  // namespace detail_0211

// Stage 5
template <typename T, typename Compare = std::less<>>
void sort_m(std::span<T> data, Compare cmp = {}, std::size_t cutoff = 16) {
    if (data.size() < 2) {
        return;
    }
    if (data.size() <= cutoff) {
        detail_0211::insertion_sort(data.data(), data.data() + data.size(), cmp);
        return;
    }
    const std::size_t mid = data.size() / 2;
    sort_m(data.first(mid), cmp, cutoff);
    sort_m(data.subspan(mid), cmp, cutoff);
    std::vector<T> scratch(mid);
    detail_0211::merge_with_scratch(data.data(), data.data() + mid, data.data() + data.size(),
                                    scratch.data(), cmp);
}

namespace detail_0211 {

template <typename T, typename Compare>
void sort_with_buffer(T* first, std::size_t count, T* scratch, Compare& cmp, std::size_t cutoff) {
    if (count < 2) {
        return;
    }
    if (count <= cutoff) {
        insertion_sort(first, first + count, cmp);
        return;
    }
    const std::size_t mid = count / 2;
    sort_with_buffer(first, mid, scratch, cmp, cutoff);
    sort_with_buffer(first + mid, count - mid, scratch, cmp, cutoff);
    merge_with_scratch(first, first + mid, first + count, scratch, cmp);
}

}  // namespace detail_0211

// Stage 6
template <typename T, typename Compare = std::less<>>
void sort_f(std::span<T> data, Compare cmp = {}, std::size_t cutoff = 16) {
    if (data.size() < 2) {
        return;
    }
    std::vector<T> scratch(data.size() / 2);
    detail_0211::sort_with_buffer(data.data(), data.size(), scratch.data(), cmp, cutoff);
}
