#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <span>
#include <vector>

struct Item {
    long long size = 0;
    long long value = 0;
};

struct Selection {
    std::vector<int> items;
    long long value = 0;
    long long size = 0;
};

namespace knapsack_detail {

inline std::vector<int> by_density(std::span<const Item> items) {
    std::vector<int> order(items.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return __int128(items[a].value) * items[b].size > __int128(items[b].value) * items[a].size;
    });
    return order;
}

inline Selection selection_of(std::span<const Item> items, std::vector<int> chosen) {
    std::sort(chosen.begin(), chosen.end());
    Selection out;
    for (int i : chosen) {
        out.value += items[i].value;
        out.size += items[i].size;
    }
    out.items = std::move(chosen);
    return out;
}

// S[v] = smallest size reaching exactly value v, for the DP in `values`.
inline std::vector<int> dp_by_value(std::span<const Item> items, const std::vector<long long>& values,
                                    long long capacity) {
    const int n = static_cast<int>(items.size());
    long long max_total = 0;
    for (int i = 0; i < n; ++i) {
        if (items[i].size <= capacity) {
            max_total += values[i];
        }
    }
    const auto limit = static_cast<std::size_t>(max_total);
    const long long kInf = std::numeric_limits<long long>::max() / 2;
    std::vector<long long> best(limit + 1, kInf);
    best[0] = 0;
    std::vector<std::vector<bool>> took(static_cast<std::size_t>(n),
                                        std::vector<bool>(limit + 1, false));
    for (int i = 0; i < n; ++i) {
        if (items[i].size > capacity) {
            continue;
        }
        const auto v = static_cast<std::size_t>(values[i]);
        for (std::size_t x = limit; x >= v; --x) {
            if (best[x - v] != kInf && best[x - v] + items[i].size < best[x]) {
                best[x] = best[x - v] + items[i].size;
                took[static_cast<std::size_t>(i)][x] = true;
            }
            if (x == 0) {
                break;
            }
        }
    }
    std::size_t bestv = 0;
    for (std::size_t x = 0; x <= limit; ++x) {
        if (best[x] <= capacity) {
            bestv = x;
        }
    }
    std::vector<int> chosen;
    std::size_t x = bestv;
    for (int i = n - 1; i >= 0; --i) {
        if (took[static_cast<std::size_t>(i)][x]) {
            chosen.push_back(i);
            x -= static_cast<std::size_t>(values[i]);
        }
    }
    return chosen;
}

}  // namespace knapsack_detail

inline Selection greedy_by_density(std::span<const Item> items, long long capacity) {
    std::vector<int> chosen;
    long long used = 0;
    for (int i : knapsack_detail::by_density(items)) {
        if (used + items[i].size <= capacity) {
            used += items[i].size;
            chosen.push_back(i);
        }
    }
    return knapsack_detail::selection_of(items, std::move(chosen));
}

struct DensityTrap {
    std::vector<Item> items;
    long long capacity = 0;
};

inline DensityTrap density_trap(long long k) {
    return {{{1, 2}, {2 * k, 2 * k}}, 2 * k};
}

inline Selection greedy_two_approx(std::span<const Item> items, long long capacity) {
    long long total = 0;
    for (const Item& item : items) {
        total += item.size;
    }
    std::vector<int> all(items.size());
    std::iota(all.begin(), all.end(), 0);
    if (total <= capacity) {
        return knapsack_detail::selection_of(items, std::move(all));
    }
    const auto order = knapsack_detail::by_density(items);
    long long size_sum = 0;
    long long value_sum = 0;
    std::vector<int> prefix;
    for (int i : order) {
        if (size_sum + items[i].size > capacity) {
            if (items[i].value > value_sum) {
                return knapsack_detail::selection_of(items, {i});
            }
            return knapsack_detail::selection_of(items, std::move(prefix));
        }
        size_sum += items[i].size;
        value_sum += items[i].value;
        prefix.push_back(i);
    }
    return knapsack_detail::selection_of(items, std::move(prefix));
}

inline Selection exact_by_value(std::span<const Item> items, long long capacity) {
    std::vector<long long> values;
    for (const Item& item : items) {
        values.push_back(item.value);
    }
    return knapsack_detail::selection_of(items, knapsack_detail::dp_by_value(items, values, capacity));
}

inline Selection knapsack_fptas(std::span<const Item> items, long long capacity, double eps) {
    const int n = static_cast<int>(items.size());
    long long max_value = 0;
    for (const Item& item : items) {
        if (item.size <= capacity) {
            max_value = std::max(max_value, item.value);
        }
    }
    if (n == 0 || max_value == 0) {
        return {};
    }
    std::vector<long long> scaled(items.size());
    const long double factor =
        static_cast<long double>(n) / (static_cast<long double>(max_value) * static_cast<long double>(eps));
    for (int i = 0; i < n; ++i) {
        scaled[i] = static_cast<long long>(std::floor(items[i].value * factor));
    }
    return knapsack_detail::selection_of(items, knapsack_detail::dp_by_value(items, scaled, capacity));
}
