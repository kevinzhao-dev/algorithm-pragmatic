#include <cstdint>
#include <limits>
#include <vector>

#include "07_heap_meter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Orders = std::vector<Order>;

std::int64_t profit_of(const Orders& orders, const std::vector<std::size_t>& chosen,
                       std::int64_t capacity, std::int64_t storage_cost) {
    std::int64_t sold = 0;
    std::int64_t revenue = 0;
    for (const std::size_t i : chosen) {
        sold += orders[i].amount;
        revenue += orders[i].price;
    }
    REQUIRE(sold <= capacity);
    return revenue - storage_cost * (capacity - sold);
}

// Tries every subset of orders that fits.
std::int64_t exhaustive(const Orders& orders, std::int64_t capacity, std::int64_t storage_cost) {
    std::int64_t best = std::numeric_limits<std::int64_t>::min();
    for (std::size_t mask = 0; mask < (std::size_t{1} << orders.size()); ++mask) {
        std::int64_t sold = 0;
        std::int64_t revenue = 0;
        for (std::size_t i = 0; i < orders.size(); ++i) {
            if ((mask >> i) & 1) {
                sold += orders[i].amount;
                revenue += orders[i].price;
            }
        }
        if (sold <= capacity) {
            best = std::max(best, revenue - storage_cost * (capacity - sold));
        }
    }
    return best;
}

Orders random_orders(Rng& rng, std::size_t max_count, std::int64_t max_amount,
                     std::int64_t max_price) {
    Orders orders(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(max_count))));
    for (Order& order : orders) {
        order = {rng.uniform(1, max_amount), rng.uniform(-max_price, max_price)};
    }
    return orders;
}

}  // namespace

TEST(1, "no capacity and no orders cost nothing") {
    CHECK_EQ(max_profit({}, 0, 5), 0);
}

TEST(1, "with no orders every unit pays storage") {
    CHECK_EQ(max_profit({}, 10, 3), -30);
}

TEST(1, "an order bigger than the capacity is ignored") {
    const Orders orders{{11, 100}};
    CHECK_EQ(max_profit(orders, 10, 1), -10);
}

TEST(1, "a profitable order is filled") {
    const Orders orders{{4, 50}};
    CHECK_EQ(max_profit(orders, 10, 1), 50 - 6);
}

TEST(1, "a negative price can still beat storage") {
    const Orders orders{{10, -2}};
    CHECK_EQ(max_profit(orders, 10, 3), -2);  // storing all ten would cost 30
}

TEST(1, "a negative price is declined when storage is cheaper") {
    const Orders orders{{10, -50}};
    CHECK_EQ(max_profit(orders, 10, 1), -10);
}

TEST(1, "greedy by price per unit is not optimal") {
    const Orders orders{{6, 12}, {5, 9}, {5, 9}};
    CHECK_EQ(max_profit(orders, 10, 0), 18);
}

TEST(1, "random orders match enumeration") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Orders orders = random_orders(rng, 12, 12, 30);
        const std::int64_t capacity = rng.uniform(0, 40);
        const std::int64_t storage = rng.uniform(0, 9);
        INFO("trial ", trial, " capacity ", capacity, " storage ", storage);
        REQUIRE_EQ(max_profit(orders, capacity, storage), exhaustive(orders, capacity, storage));
    }
}

PERF(1, "two thousand orders and capacity two hundred thousand", 2000) {
    Rng rng;
    Orders orders(2000);
    for (Order& order : orders) {
        order = {rng.uniform(1, 500), rng.uniform(-200, 1000)};
    }
    CHECK_GT(max_profit(orders, 200'000, 2), 0);
}

TEST(2, "no orders mean none are chosen") {
    CHECK(chosen_orders({}, 10, 3).empty());
}

TEST(2, "a losing order stays unchosen") {
    const Orders orders{{10, -50}};
    CHECK(chosen_orders(orders, 10, 1).empty());
}

TEST(2, "a costly-to-store plan chooses the negative order") {
    const Orders orders{{10, -2}};
    CHECK_EQ(chosen_orders(orders, 10, 3), std::vector<std::size_t>{0});
}

TEST(2, "random selections are valid and optimal") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Orders orders = random_orders(rng, 12, 12, 30);
        const std::int64_t capacity = rng.uniform(0, 40);
        const std::int64_t storage = rng.uniform(0, 9);
        INFO("trial ", trial, " capacity ", capacity, " storage ", storage);
        const auto chosen = chosen_orders(orders, capacity, storage);
        for (std::size_t k = 0; k < chosen.size(); ++k) {
            REQUIRE(chosen[k] < orders.size());
            REQUIRE(k == 0 || chosen[k - 1] < chosen[k]);
        }
        REQUIRE_EQ(profit_of(orders, chosen, capacity, storage), exhaustive(orders, capacity, storage));
    }
}

TEST(3, "the curve starts at zero capacity") {
    const Orders orders{{2, 10}};
    const auto curve = profit_curve(orders, 3, 1);
    CHECK_EQ(curve, std::vector<std::int64_t>{0, -1, 10, 9});
}

TEST(3, "an empty curve still has one entry") {
    CHECK_EQ(profit_curve({}, 0, 7), std::vector<std::int64_t>{0});
}

TEST(3, "every point on the curve matches enumeration") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const Orders orders = random_orders(rng, 10, 10, 30);
        const std::int64_t capacity = rng.uniform(0, 30);
        const std::int64_t storage = rng.uniform(0, 9);
        const auto curve = profit_curve(orders, capacity, storage);
        REQUIRE_EQ(curve.size(), static_cast<std::size_t>(capacity) + 1);
        for (std::int64_t j = 0; j <= capacity; ++j) {
            INFO("trial ", trial, " j ", j);
            REQUIRE_EQ(curve[static_cast<std::size_t>(j)], exhaustive(orders, j, storage));
        }
    }
}

TEST(3, "max_profit and the curve use memory linear in the capacity") {
    Rng rng;
    Orders orders(500);
    for (Order& order : orders) {
        order = {rng.uniform(1, 300), rng.uniform(1, 500)};
    }
    const std::int64_t capacity = 50'000;
    {
        heap_meter::Scope scope;
        CHECK_GT(max_profit(orders, capacity, 1), 0);
        CHECK_LE(scope.peak_bytes(), 24 * static_cast<std::size_t>(capacity));
    }
    {
        heap_meter::Scope scope;
        CHECK_EQ(profit_curve(orders, capacity, 1).size(), capacity + 1);
        CHECK_LE(scope.peak_bytes(), 24 * static_cast<std::size_t>(capacity));
    }
}

PERF(3, "the curve in pseudo-polynomial time", 1500) {
    Rng rng;
    Orders orders(1500);
    for (Order& order : orders) {
        order = {rng.uniform(1, 500), rng.uniform(1, 1000)};
    }
    CHECK_EQ(profit_curve(orders, 300'000, 3).size(), 300'001);
}
