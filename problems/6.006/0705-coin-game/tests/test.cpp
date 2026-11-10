#include <algorithm>
#include <cstdint>
#include <numeric>
#include <span>
#include <vector>

#include "07_heap_meter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Coins = std::vector<int>;

// Exhaustive game tree. mover 0 tries to maximize player 0's total, mover 1 to minimize it.
std::int64_t minimax(const Coins& coins, std::size_t first, std::size_t last, int mover) {
    if (first == last) {
        return 0;
    }
    const std::int64_t from_front =
        (mover == 0 ? coins[first] : 0) + minimax(coins, first + 1, last, 1 - mover);
    const std::int64_t from_back =
        (mover == 0 ? coins[last - 1] : 0) + minimax(coins, first, last - 1, 1 - mover);
    return mover == 0 ? std::max(from_front, from_back) : std::min(from_front, from_back);
}

std::int64_t optimal(const Coins& coins) {
    return minimax(coins, 0, coins.size(), 0);
}

// The fewest coins worth the bot can end up with over every line the opponent can play.
std::int64_t worst_case(const CoinGame& game, const Coins& coins, std::size_t first,
                        std::size_t last, bool bot_moves) {
    if (first == last) {
        return 0;
    }
    if (bot_moves) {
        return game.best_move(first, last - 1) == Side::Front
                   ? coins[first] + worst_case(game, coins, first + 1, last, false)
                   : coins[last - 1] + worst_case(game, coins, first, last - 1, false);
    }
    return std::min(worst_case(game, coins, first + 1, last, true),
                    worst_case(game, coins, first, last - 1, true));
}

Coins random_coins(Rng& rng, std::size_t max_length) {
    return rng.ints(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(max_length))),
                    -20, 40);
}

}  // namespace

TEST(1, "no coins means no value") {
    CHECK_EQ(coin_game_value({}), 0);
}

TEST(1, "a single coin goes to the first player even if negative") {
    const Coins coins{-5};
    CHECK_EQ(coin_game_value(coins), -5);
}

TEST(1, "two coins: the first player keeps the larger one") {
    const Coins coins{3, 8};
    CHECK_EQ(coin_game_value(coins), 8);
}

TEST(1, "greedily taking the larger end is not optimal") {
    const Coins coins{1, 1, 1, 2, 9, 2};
    CHECK_EQ(coin_game_value(coins), 11);
}

TEST(1, "totals beyond 32 bits are handled") {
    const Coins coins(5000, 1'000'000);
    CHECK_EQ(coin_game_value(coins), std::int64_t{2'500'000'000});
}

TEST(1, "random games match the exhaustive game tree") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Coins coins = random_coins(rng, 12);
        INFO("coins ", coins);
        REQUIRE_EQ(coin_game_value(coins), optimal(coins));
    }
}

PERF(1, "six thousand coins in quadratic time", 500) {
    Rng rng;
    const Coins coins = rng.ints(6000, -1000, 1000);
    const std::int64_t total = std::accumulate(coins.begin(), coins.end(), std::int64_t{0});
    CHECK_GE(coin_game_value(coins) * 2, total);
}

TEST(2, "value agrees with the exhaustive game tree") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        const Coins coins = random_coins(rng, 12);
        INFO("coins ", coins);
        REQUIRE_EQ(CoinGame(coins).value(), optimal(coins));
    }
}

TEST(2, "an empty game has value zero") {
    CHECK_EQ(CoinGame({}).value(), 0);
}

TEST(2, "with one coin left both sides are the same coin") {
    const Coins coins{4, 5};
    const CoinGame game(coins);
    CHECK(game.best_move(0, 0) == Side::Front || game.best_move(0, 0) == Side::Back);
}

TEST(2, "the bot secures at least the game value against every opponent line") {
    Rng rng;
    for (int trial = 0; trial < 600; ++trial) {
        const Coins coins = random_coins(rng, 14);
        const CoinGame game(coins);
        INFO("coins ", coins);
        // The optimal opponent is one of the lines, so the worst case is exactly the value.
        REQUIRE_EQ(worst_case(game, coins, 0, coins.size(), true), game.value());
    }
}

TEST(2, "best_move is optimal for every sub-range") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const Coins coins = random_coins(rng, 9);
        const CoinGame game(coins);
        for (std::size_t first = 0; first < coins.size(); ++first) {
            for (std::size_t last = first; last < coins.size(); ++last) {
                INFO("coins ", coins, " range ", first, "..", last);
                const Coins range(coins.begin() + static_cast<std::ptrdiff_t>(first),
                                  coins.begin() + static_cast<std::ptrdiff_t>(last) + 1);
                const bool front = game.best_move(first, last) == Side::Front;
                const Coins rest = front ? Coins(range.begin() + 1, range.end())
                                         : Coins(range.begin(), range.end() - 1);
                const std::int64_t rest_total =
                    std::accumulate(rest.begin(), rest.end(), std::int64_t{0});
                const std::int64_t taken = front ? range.front() : range.back();
                REQUIRE_EQ(taken + rest_total - optimal(rest), optimal(range));
            }
        }
    }
}

TEST(3, "coin_game_value keeps memory linear") {
    Rng rng;
    const Coins coins = rng.ints(3000, -1000, 1000);
    heap_meter::Scope scope;
    const std::int64_t value = coin_game_value(coins);
    CHECK_LE(scope.peak_bytes(), 40 * coins.size());
    CHECK_EQ(value, CoinGame(coins).value());
}

TEST(3, "the rolling table still matches the game tree") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        const Coins coins = random_coins(rng, 11);
        INFO("coins ", coins);
        REQUIRE_EQ(coin_game_value(coins), optimal(coins));
    }
}
