#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <span>
#include <vector>

struct Item {
    std::int64_t size;
    std::int64_t value;
};

inline std::int64_t knapsack_value(std::span<const Item> items, std::int64_t capacity) {
    const auto room = static_cast<std::size_t>(capacity);
    std::vector<std::int64_t> best(room + 1, 0);
    for (const Item& item : items) {
        // Descending, so best[j - size] still excludes this item.
        for (std::int64_t j = capacity; j >= item.size; --j) {
            best[static_cast<std::size_t>(j)] =
                std::max(best[static_cast<std::size_t>(j)],
                         item.value + best[static_cast<std::size_t>(j - item.size)]);
        }
    }
    return best[room];
}

inline std::vector<std::size_t> knapsack_items(std::span<const Item> items, std::int64_t capacity) {
    const std::size_t n = items.size();
    const auto room = static_cast<std::size_t>(capacity);
    std::vector<std::int64_t> best(room + 1, 0);
    // take[i][j]: item i is used in the best choice of items 0..i for capacity j.
    std::vector<bool> take(n * (room + 1), false);
    for (std::size_t i = 0; i < n; ++i) {
        const Item& item = items[i];
        for (std::int64_t j = capacity; j >= item.size; --j) {
            const std::int64_t with =
                item.value + best[static_cast<std::size_t>(j - item.size)];
            if (with > best[static_cast<std::size_t>(j)]) {
                best[static_cast<std::size_t>(j)] = with;
                take[i * (room + 1) + static_cast<std::size_t>(j)] = true;
            }
        }
    }

    std::vector<std::size_t> chosen;
    std::int64_t j = capacity;
    for (std::size_t i = n; i-- > 0;) {
        if (take[i * (room + 1) + static_cast<std::size_t>(j)]) {
            chosen.push_back(i);
            j -= items[i].size;
        }
    }
    std::ranges::reverse(chosen);
    return chosen;
}

inline std::int64_t knapsack_by_value(std::span<const Item> items, std::int64_t capacity) {
    constexpr std::int64_t kImpossible = std::numeric_limits<std::int64_t>::max();
    std::int64_t total = 0;
    for (const Item& item : items) {
        total += item.value;
    }
    // least_size[v]: the smallest total size that achieves exactly value v.
    std::vector<std::int64_t> least_size(static_cast<std::size_t>(total) + 1, kImpossible);
    least_size[0] = 0;
    for (const Item& item : items) {
        for (std::int64_t v = total; v >= item.value; --v) {
            const std::int64_t below = least_size[static_cast<std::size_t>(v - item.value)];
            if (below != kImpossible) {
                least_size[static_cast<std::size_t>(v)] =
                    std::min(least_size[static_cast<std::size_t>(v)], below + item.size);
            }
        }
    }
    for (std::int64_t v = total; v > 0; --v) {
        if (least_size[static_cast<std::size_t>(v)] <= capacity) {
            return v;
        }
    }
    return 0;
}
