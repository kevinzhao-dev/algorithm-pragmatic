#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

inline std::vector<std::int64_t> rod_best(std::span<const std::int64_t> prices,
                                          std::vector<std::size_t>* first_piece) {
    const std::size_t length = prices.empty() ? 0 : prices.size() - 1;
    std::vector<std::int64_t> best(length + 1, 0);
    if (first_piece != nullptr) {
        first_piece->assign(length + 1, 0);
    }
    for (std::size_t l = 1; l <= length; ++l) {
        for (std::size_t piece = 1; piece <= l; ++piece) {
            const std::int64_t candidate = prices[piece] + best[l - piece];
            if (candidate > best[l] || piece == 1) {
                best[l] = candidate;
                if (first_piece != nullptr) {
                    (*first_piece)[l] = piece;
                }
            }
        }
    }
    return best;
}

inline std::int64_t rod_cut_value(std::span<const std::int64_t> prices) {
    return rod_best(prices, nullptr).back();
}

inline std::vector<std::size_t> rod_cut_pieces(std::span<const std::int64_t> prices) {
    std::vector<std::size_t> first_piece;
    rod_best(prices, &first_piece);
    std::vector<std::size_t> pieces;
    for (std::size_t l = first_piece.size() - 1; l > 0; l -= first_piece[l]) {
        pieces.push_back(first_piece[l]);
    }
    return pieces;
}

struct Item {
    std::size_t size;
    std::int64_t value;
};

inline std::int64_t unbounded_knapsack(std::span<const Item> items, std::size_t capacity) {
    std::vector<std::int64_t> best(capacity + 1, 0);
    for (std::size_t room = 1; room <= capacity; ++room) {
        for (const Item& item : items) {
            if (item.size <= room) {
                best[room] = std::max(best[room], item.value + best[room - item.size]);
            }
        }
    }
    return best[capacity];
}

inline std::optional<std::int64_t> min_coins(std::span<const int> coins, int amount) {
    constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
    std::vector<std::int64_t> fewest(static_cast<std::size_t>(amount) + 1, kUnreachable);
    fewest[0] = 0;
    for (int value = 1; value <= amount; ++value) {
        for (const int coin : coins) {
            if (coin <= value && fewest[static_cast<std::size_t>(value - coin)] != kUnreachable) {
                fewest[static_cast<std::size_t>(value)] =
                    std::min(fewest[static_cast<std::size_t>(value)],
                             fewest[static_cast<std::size_t>(value - coin)] + 1);
            }
        }
    }
    const std::int64_t answer = fewest[static_cast<std::size_t>(amount)];
    if (answer == kUnreachable) {
        return std::nullopt;
    }
    return answer;
}

inline std::uint64_t count_change_ways(std::span<const int> coins, int amount) {
    // Coins on the outer loop: each multiset is built in one fixed coin order.
    std::vector<std::uint64_t> ways(static_cast<std::size_t>(amount) + 1, 0);
    ways[0] = 1;
    for (const int coin : coins) {
        for (int value = coin; value <= amount; ++value) {
            ways[static_cast<std::size_t>(value)] += ways[static_cast<std::size_t>(value - coin)];
        }
    }
    return ways[static_cast<std::size_t>(amount)];
}

inline std::uint64_t count_ordered_ways(std::span<const int> coins, int amount) {
    // Amounts on the outer loop: the coin chosen last can be any of them at every step.
    std::vector<std::uint64_t> ways(static_cast<std::size_t>(amount) + 1, 0);
    ways[0] = 1;
    for (int value = 1; value <= amount; ++value) {
        for (const int coin : coins) {
            if (coin <= value) {
                ways[static_cast<std::size_t>(value)] +=
                    ways[static_cast<std::size_t>(value - coin)];
            }
        }
    }
    return ways[static_cast<std::size_t>(amount)];
}

struct Offer {
    std::int64_t units;
    std::int64_t price;
};

struct Purchase {
    std::int64_t cost;
    std::vector<std::int64_t> counts;
};

inline std::optional<Purchase> cheapest_capacity(std::span<const Offer> offers,
                                                 std::int64_t demand) {
    constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
    const auto needed = static_cast<std::size_t>(demand);
    std::vector<std::int64_t> cost(needed + 1, kUnreachable);
    std::vector<std::size_t> choice(needed + 1, offers.size());
    cost[0] = 0;
    for (std::size_t units = 1; units <= needed; ++units) {
        for (std::size_t i = 0; i < offers.size(); ++i) {
            const auto covered = static_cast<std::size_t>(offers[i].units);
            const std::size_t rest = covered >= units ? 0 : units - covered;
            if (cost[rest] != kUnreachable && cost[rest] + offers[i].price < cost[units]) {
                cost[units] = cost[rest] + offers[i].price;
                choice[units] = i;
            }
        }
    }
    if (cost[needed] == kUnreachable) {
        return std::nullopt;
    }
    Purchase purchase{cost[needed], std::vector<std::int64_t>(offers.size(), 0)};
    for (std::size_t units = needed; units > 0;) {
        const std::size_t i = choice[units];
        ++purchase.counts[i];
        const auto covered = static_cast<std::size_t>(offers[i].units);
        units = covered >= units ? 0 : units - covered;
    }
    return purchase;
}
