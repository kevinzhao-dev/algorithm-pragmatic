#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Prices = std::vector<std::int64_t>;

// Tries every way to cut the remaining length.
std::int64_t exhaustive_rod(const Prices& prices, std::size_t length) {
    if (length == 0) {
        return 0;
    }
    std::int64_t best = 0;
    for (std::size_t piece = 1; piece <= length; ++piece) {
        best = std::max(best, prices[piece] + exhaustive_rod(prices, length - piece));
    }
    return best;
}

Prices random_prices(Rng& rng, std::size_t max_length) {
    Prices prices(static_cast<std::size_t>(rng.uniform(1, static_cast<std::int64_t>(max_length)) + 1));
    prices[0] = 0;
    for (std::size_t l = 1; l < prices.size(); ++l) {
        prices[l] = rng.uniform(0, 30);
    }
    return prices;
}

std::int64_t exhaustive_unbounded(std::span<const Item> items, std::size_t room) {
    std::int64_t best = 0;
    for (const Item& item : items) {
        if (item.size <= room) {
            best = std::max(best, item.value + exhaustive_unbounded(items, room - item.size));
        }
    }
    return best;
}

std::vector<Item> random_items(Rng& rng, std::size_t max_count, std::size_t max_size) {
    std::vector<Item> items(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(max_count))));
    for (Item& item : items) {
        item = {static_cast<std::size_t>(rng.uniform(1, static_cast<std::int64_t>(max_size))),
                rng.uniform(0, 25)};
    }
    return items;
}

// Fewest coins, by trying every coin at every step.
std::optional<std::int64_t> exhaustive_min_coins(const std::vector<int>& coins, int amount) {
    if (amount == 0) {
        return 0;
    }
    std::optional<std::int64_t> best;
    for (const int coin : coins) {
        if (coin <= amount) {
            const auto rest = exhaustive_min_coins(coins, amount - coin);
            if (rest && (!best || *rest + 1 < *best)) {
                best = *rest + 1;
            }
        }
    }
    return best;
}

std::uint64_t exhaustive_ordered(const std::vector<int>& coins, int amount) {
    if (amount == 0) {
        return 1;
    }
    std::uint64_t total = 0;
    for (const int coin : coins) {
        if (coin <= amount) {
            total += exhaustive_ordered(coins, amount - coin);
        }
    }
    return total;
}

// Counts multisets by never going back to a coin earlier than the current one.
std::uint64_t exhaustive_multisets(const std::vector<int>& coins, std::size_t from, int amount) {
    if (amount == 0) {
        return 1;
    }
    std::uint64_t total = 0;
    for (std::size_t i = from; i < coins.size(); ++i) {
        if (coins[i] <= amount) {
            total += exhaustive_multisets(coins, i, amount - coins[i]);
        }
    }
    return total;
}

std::vector<int> random_coins(Rng& rng, std::size_t max_count) {
    std::vector<int> coins;
    for (int i = 1; i <= 12; ++i) {
        if (rng.coin(0.4)) {
            coins.push_back(i);
        }
    }
    if (coins.size() > max_count) {
        coins.resize(max_count);
    }
    return coins;
}

std::int64_t exhaustive_capacity(const std::vector<Offer>& offers, std::int64_t demand,
                                 std::size_t i = 0) {
    constexpr std::int64_t kNone = std::numeric_limits<std::int64_t>::max() / 4;
    if (demand <= 0) {
        return 0;
    }
    if (i == offers.size()) {
        return kNone;
    }
    std::int64_t best = kNone;
    for (std::int64_t copies = 0;; ++copies) {
        const std::int64_t rest = demand - copies * offers[i].units;
        best = std::min(best, copies * offers[i].price + exhaustive_capacity(offers, rest, i + 1));
        if (rest <= 0) {
            break;
        }
    }
    return best;
}

std::vector<Offer> random_offers(Rng& rng) {
    std::vector<Offer> offers(static_cast<std::size_t>(rng.uniform(1, 4)));
    for (Offer& offer : offers) {
        offer = {rng.uniform(1, 9), rng.uniform(0, 40)};
    }
    return offers;
}

}  // namespace

TEST(1, "no rod earns nothing") {
    CHECK_EQ(rod_cut_value({}), 0);
    const Prices only_zero{0};
    CHECK_EQ(rod_cut_value(only_zero), 0);
}

TEST(1, "selling the whole piece can be best") {
    const Prices prices{0, 1, 2, 9};
    CHECK_EQ(rod_cut_value(prices), 9);
}

TEST(1, "cutting can beat selling whole") {
    const Prices prices{0, 3, 4};
    CHECK_EQ(rod_cut_value(prices), 6);  // two pieces of length 1
}

TEST(1, "the best price per unit is not always the best plan") {
    // Length 3 has the best ratio (8 / 3), but using it forces a leftover piece of length 1.
    const Prices prices{0, 1, 5, 8, 9};
    CHECK_EQ(rod_cut_value(prices), 5 + 5);
}

TEST(1, "random prices match exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        const Prices prices = random_prices(rng, 11);
        INFO("trial ", trial, " prices ", prices);
        REQUIRE_EQ(rod_cut_value(prices), exhaustive_rod(prices, prices.size() - 1));
    }
}

PERF(1, "a rod of length twelve thousand", 600) {
    Rng rng;
    Prices prices(12'001);
    for (std::size_t l = 1; l < prices.size(); ++l) {
        prices[l] = rng.uniform(1, 1000);
    }
    CHECK_GT(rod_cut_value(prices), 0);
}

TEST(2, "pieces add up to the rod and earn the optimum") {
    const Prices prices{0, 1, 5, 8, 9, 10};
    const auto pieces = rod_cut_pieces(prices);
    std::size_t total = 0;
    std::int64_t earned = 0;
    for (const std::size_t piece : pieces) {
        total += piece;
        earned += prices[piece];
    }
    CHECK_EQ(total, 5);
    CHECK_EQ(earned, rod_cut_value(prices));
}

TEST(2, "no rod means no pieces") {
    CHECK(rod_cut_pieces({}).empty());
    const Prices only_zero{0};
    CHECK(rod_cut_pieces(only_zero).empty());
}

TEST(2, "random pieces are valid and optimal") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        const Prices prices = random_prices(rng, 15);
        const auto pieces = rod_cut_pieces(prices);
        std::size_t total = 0;
        std::int64_t earned = 0;
        for (const std::size_t piece : pieces) {
            REQUIRE(piece >= 1);
            REQUIRE(piece < prices.size());
            total += piece;
            earned += prices[piece];
        }
        INFO("trial ", trial, " prices ", prices);
        REQUIRE_EQ(total, prices.size() - 1);
        REQUIRE_EQ(earned, rod_cut_value(prices));
    }
}

TEST(3, "no items or no capacity earn nothing") {
    const std::vector<Item> items{{2, 5}};
    CHECK_EQ(unbounded_knapsack({}, 10), 0);
    CHECK_EQ(unbounded_knapsack(items, 0), 0);
    CHECK_EQ(unbounded_knapsack(items, 1), 0);
}

TEST(3, "items can be used repeatedly") {
    const std::vector<Item> items{{3, 5}};
    CHECK_EQ(unbounded_knapsack(items, 10), 15);
}

TEST(3, "capacity does not have to be used up") {
    const std::vector<Item> items{{4, 9}, {6, 10}};
    CHECK_EQ(unbounded_knapsack(items, 7), 10);
}

TEST(3, "random items match exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        const auto items = random_items(rng, 4, 8);
        const auto capacity = static_cast<std::size_t>(rng.uniform(0, 18));
        INFO("trial ", trial, " capacity ", capacity);
        REQUIRE_EQ(unbounded_knapsack(items, capacity), exhaustive_unbounded(items, capacity));
    }
}

PERF(3, "a hundred items and a million capacity", 1500) {
    Rng rng;
    std::vector<Item> items(100);
    for (Item& item : items) {
        item = {static_cast<std::size_t>(rng.uniform(1, 1000)), rng.uniform(1, 1000)};
    }
    CHECK_GT(unbounded_knapsack(items, 1'000'000), 0);
}

TEST(4, "amount zero takes no coins and one way") {
    const std::vector<int> coins{2, 5};
    CHECK_EQ(min_coins(coins, 0), std::optional<std::int64_t>(0));
    CHECK_EQ(count_change_ways(coins, 0), 1);
    CHECK_EQ(count_ordered_ways(coins, 0), 1);
}

TEST(4, "an impossible amount is reported") {
    const std::vector<int> coins{4, 6};
    CHECK_EQ(min_coins(coins, 7), std::optional<std::int64_t>());
    CHECK_EQ(count_change_ways(coins, 7), 0);
    CHECK_EQ(count_ordered_ways(coins, 7), 0);
}

TEST(4, "greedy on the largest coin is not always best") {
    const std::vector<int> coins{1, 3, 4};
    CHECK_EQ(min_coins(coins, 6), std::optional<std::int64_t>(2));  // 3 + 3, not 4 + 1 + 1
}

TEST(4, "unordered and ordered counts differ") {
    const std::vector<int> coins{1, 2};
    CHECK_EQ(count_change_ways(coins, 3), 2);   // 1+1+1, 1+2
    CHECK_EQ(count_ordered_ways(coins, 3), 3);  // 1+1+1, 1+2, 2+1
}

TEST(4, "random coin systems match enumeration") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const auto coins = random_coins(rng, 5);
        const int amount = static_cast<int>(rng.uniform(0, 14));
        INFO("trial ", trial, " coins ", coins, " amount ", amount);
        REQUIRE_EQ(min_coins(coins, amount), exhaustive_min_coins(coins, amount));
        REQUIRE_EQ(count_change_ways(coins, amount), exhaustive_multisets(coins, 0, amount));
        REQUIRE_EQ(count_ordered_ways(coins, amount), exhaustive_ordered(coins, amount));
    }
}

TEST(5, "no demand costs nothing") {
    const std::vector<Offer> offers{{10, 7}};
    const auto purchase = cheapest_capacity(offers, 0);
    REQUIRE(purchase.has_value());
    CHECK_EQ(purchase->cost, 0);
    CHECK_EQ(purchase->counts, std::vector<std::int64_t>{0});
}

TEST(5, "demand without offers cannot be met") {
    CHECK(!cheapest_capacity({}, 5).has_value());
    const auto zero = cheapest_capacity({}, 0);
    REQUIRE(zero.has_value());
    CHECK_EQ(zero->cost, 0);
}

TEST(5, "overbuying can be cheapest") {
    const std::vector<Offer> offers{{4, 10}, {1, 6}};
    const auto purchase = cheapest_capacity(offers, 3);
    REQUIRE(purchase.has_value());
    CHECK_EQ(purchase->cost, 10);
    CHECK_EQ(purchase->counts, std::vector<std::int64_t>{1, 0});
}

TEST(5, "random offers match exhaustive search and the counts add up") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const auto offers = random_offers(rng);
        const std::int64_t demand = rng.uniform(0, 16);
        INFO("trial ", trial, " demand ", demand);
        const auto purchase = cheapest_capacity(offers, demand);
        REQUIRE(purchase.has_value());
        REQUIRE_EQ(purchase->counts.size(), offers.size());
        std::int64_t cost = 0;
        std::int64_t units = 0;
        for (std::size_t i = 0; i < offers.size(); ++i) {
            REQUIRE(purchase->counts[i] >= 0);
            cost += purchase->counts[i] * offers[i].price;
            units += purchase->counts[i] * offers[i].units;
        }
        REQUIRE(units >= demand);
        REQUIRE_EQ(cost, purchase->cost);
        REQUIRE_EQ(purchase->cost, exhaustive_capacity(offers, demand));
    }
}

PERF(5, "a large demand stays O(offers times demand)", 1500) {
    Rng rng;
    std::vector<Offer> offers(100);
    for (Offer& offer : offers) {
        offer = {rng.uniform(1, 1000), rng.uniform(1, 1000)};
    }
    CHECK(cheapest_capacity(offers, 2'000'000).has_value());
}
