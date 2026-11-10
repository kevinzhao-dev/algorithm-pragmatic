#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

// Stable. Items may be move-only and need not be default-constructible.
template <typename T, typename KeyFn>
void counting_sort(std::span<T> items, std::size_t u, KeyFn key) {
    std::vector<std::size_t> next(u + 1, 0);
    for (const T& item : items) {
        ++next[static_cast<std::size_t>(key(item)) + 1];
    }
    for (std::size_t i = 0; i < u; ++i) {
        next[i + 1] += next[i];
    }
    std::vector<std::optional<T>> buffer(items.size());
    for (T& item : items) {
        buffer[next[static_cast<std::size_t>(key(item))]++].emplace(std::move(item));
    }
    for (std::size_t i = 0; i < items.size(); ++i) {
        items[i] = std::move(*buffer[i]);
    }
}

// Lexicographic sort on `digits` columns; digit(item, 0) is the least significant column.
template <typename T, typename DigitFn>
void tuple_sort(std::span<T> items, std::size_t digits, std::size_t base, DigitFn digit) {
    for (std::size_t column = 0; column < digits; ++column) {
        counting_sort(items, base, [&](const T& item) { return digit(item, column); });
    }
}

// Base-n digits make the running time O(n * ceil(log_n(max_key + 1))). Keys are read once and
// the digit passes run over (key, index) pairs, so large items are moved only at the end.
template <typename T, typename KeyFn>
void radix_sort(std::span<T> items, KeyFn key) {
    const std::size_t n = items.size();
    if (n < 2) {
        return;
    }
    struct Entry {
        std::uint64_t key;
        std::size_t index;
    };
    std::vector<Entry> entries(n);
    std::uint64_t max_key = 0;
    for (std::size_t i = 0; i < n; ++i) {
        entries[i] = Entry{static_cast<std::uint64_t>(key(items[i])), i};
        max_key = std::max(max_key, entries[i].key);
    }

    const std::uint64_t base = n;
    std::uint64_t place = 1;
    while (true) {
        counting_sort<Entry>(entries, static_cast<std::size_t>(base), [&](const Entry& entry) {
            return static_cast<std::size_t>((entry.key / place) % base);
        });
        if (place > max_key / base) {  // the next place would exceed every key (or overflow)
            break;
        }
        place *= base;
    }

    std::vector<std::optional<T>> buffer(n);
    for (std::size_t i = 0; i < n; ++i) {
        buffer[i].emplace(std::move(items[entries[i].index]));
    }
    for (std::size_t i = 0; i < n; ++i) {
        items[i] = std::move(*buffer[i]);
    }
}

inline void sort_signed(std::span<std::int64_t> values) {
    radix_sort(values, [](std::int64_t value) {
        return static_cast<std::uint64_t>(value) ^ (std::uint64_t{1} << 63);
    });
}

inline void sort_equal_length(std::span<std::string> strings) {
    if (strings.empty()) {
        return;
    }
    const std::size_t length = strings.front().size();
    tuple_sort(strings, length, 256, [length](const std::string& text, std::size_t column) {
        return static_cast<unsigned char>(text[length - 1 - column]);
    });
}
