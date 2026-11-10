#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

struct Order {
    std::int64_t amount;
    std::int64_t price;
};

inline std::vector<std::int64_t> profit_curve(std::span<const Order> orders, std::int64_t capacity,
                                              std::int64_t storage_cost) {
    // With no orders, every unit of capacity sits unsold.
    std::vector<std::int64_t> best(static_cast<std::size_t>(capacity) + 1);
    for (std::int64_t j = 0; j <= capacity; ++j) {
        best[static_cast<std::size_t>(j)] = -storage_cost * j;
    }
    for (const Order& order : orders) {
        for (std::int64_t j = capacity; j >= order.amount; --j) {
            best[static_cast<std::size_t>(j)] =
                std::max(best[static_cast<std::size_t>(j)],
                         order.price + best[static_cast<std::size_t>(j - order.amount)]);
        }
    }
    return best;
}

inline std::int64_t max_profit(std::span<const Order> orders, std::int64_t capacity,
                               std::int64_t storage_cost) {
    return profit_curve(orders, capacity, storage_cost)[static_cast<std::size_t>(capacity)];
}

inline std::vector<std::size_t> chosen_orders(std::span<const Order> orders, std::int64_t capacity,
                                              std::int64_t storage_cost) {
    const std::size_t n = orders.size();
    const auto room = static_cast<std::size_t>(capacity);
    std::vector<std::int64_t> best(room + 1);
    for (std::int64_t j = 0; j <= capacity; ++j) {
        best[static_cast<std::size_t>(j)] = -storage_cost * j;
    }
    std::vector<bool> take(n * (room + 1), false);
    for (std::size_t i = 0; i < n; ++i) {
        const Order& order = orders[i];
        for (std::int64_t j = capacity; j >= order.amount; --j) {
            const std::int64_t with =
                order.price + best[static_cast<std::size_t>(j - order.amount)];
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
            j -= orders[i].amount;
        }
    }
    std::ranges::reverse(chosen);
    return chosen;
}
