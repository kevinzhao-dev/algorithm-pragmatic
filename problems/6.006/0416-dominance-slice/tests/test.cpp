#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Answer = std::optional<std::pair<std::int64_t, std::int64_t>>;

std::int64_t slice_value(std::span<const Topping> points, std::int64_t x, std::int64_t y) {
    std::int64_t total = 0;
    for (const Topping& point : points) {
        if (point.x <= x && point.y <= y) {
            total += point.t;
        }
    }
    return total;
}

std::optional<std::int64_t> brute_best(std::span<const Topping> points) {
    std::set<std::int64_t> xs, ys;
    for (const Topping& point : points) {
        xs.insert(point.x);
        ys.insert(point.y);
    }
    std::optional<std::int64_t> best;
    for (std::int64_t x : xs) {
        for (std::int64_t y : ys) {
            const std::int64_t value = slice_value(points, x, y);
            if (value > 0 && (!best || value > *best)) {
                best = value;
            }
        }
    }
    return best;
}

void check_slice(std::span<const Topping> points) {
    const auto got = tastiest_slice(points);
    const auto best = brute_best(points);
    REQUIRE_EQ(got.has_value(), best.has_value());
    if (got.has_value()) {
        CHECK_EQ(got->t, *best);
        CHECK_EQ(slice_value(points, got->x, got->y), got->t);
        const auto on_axis = [&](auto member, std::int64_t coordinate) {
            return std::any_of(points.begin(), points.end(), [&](const Topping& p) { return p.*member == coordinate; });
        };
        CHECK(on_axis(&Topping::x, got->x));
        CHECK(on_axis(&Topping::y, got->y));
    }
}

void check_max_prefix(const PrefixTree& tree, const std::map<std::int64_t, std::int64_t>& model) {
    const auto got = tree.max_prefix();
    REQUIRE_EQ(got.has_value(), !model.empty());
    if (!got.has_value()) {
        return;
    }
    const auto [key, value] = *got;
    std::int64_t running = 0;
    std::int64_t best = std::numeric_limits<std::int64_t>::min();
    std::optional<std::int64_t> at_key;
    for (const auto& [k, v] : model) {
        running += v;
        best = std::max(best, running);
        if (k == key) {
            at_key = running;
        }
    }
    REQUIRE(at_key.has_value());  // k* must be a stored key
    REQUIRE_EQ(*at_key, value);   // it reports its own prefix sum
    REQUIRE_EQ(value, best);      // and that sum is the maximum
}

}  // namespace

TEST(1, "an empty tree has no prefix") {
    const PrefixTree tree;
    CHECK_EQ(tree.size(), 0);
    CHECK_EQ(tree.max_prefix(), Answer());
    CHECK_EQ(tree.value_of(1), std::optional<std::int64_t>());
}

TEST(1, "a single key is its own prefix") {
    PrefixTree tree;
    CHECK(tree.insert(5, 7));
    CHECK_EQ(tree.max_prefix(), Answer(std::pair<std::int64_t, std::int64_t>{5, 7}));
    PrefixTree negative;
    negative.insert(3, -4);
    CHECK_EQ(negative.max_prefix(), Answer(std::pair<std::int64_t, std::int64_t>{3, -4}));
}

TEST(1, "the maximum may come from the middle, the end, or the start") {
    PrefixTree tree;
    tree.insert(1, 5);
    tree.insert(2, -7);
    tree.insert(3, 4);
    // prefix(1) = 5, prefix(2) = -2, prefix(3) = 2: best is at key 1.
    CHECK_EQ(tree.max_prefix(), Answer(std::pair<std::int64_t, std::int64_t>{1, 5}));

    PrefixTree grows;
    grows.insert(1, 1);
    grows.insert(2, 2);
    grows.insert(3, 3);
    CHECK_EQ(grows.max_prefix(), Answer(std::pair<std::int64_t, std::int64_t>{3, 6}));
}

TEST(1, "overwriting and erasing change the answer") {
    PrefixTree tree;
    tree.insert(1, 5);
    tree.insert(2, 5);
    CHECK_EQ(tree.max_prefix(), Answer(std::pair<std::int64_t, std::int64_t>{2, 10}));
    CHECK(!tree.insert(2, -100));  // existing key: value replaced
    CHECK_EQ(tree.max_prefix(), Answer(std::pair<std::int64_t, std::int64_t>{1, 5}));
    CHECK(tree.erase(1));
    CHECK_EQ(tree.max_prefix(), Answer(std::pair<std::int64_t, std::int64_t>{2, -100}));
    CHECK(!tree.erase(99));
    CHECK_EQ(tree.size(), 1);
}

TEST(1, "value_of reports what is stored") {
    PrefixTree tree;
    tree.insert(2, 20);
    CHECK_EQ(tree.value_of(2), std::optional<std::int64_t>(20));
    CHECK_EQ(tree.value_of(3), std::optional<std::int64_t>());
}

TEST(1, "random operations match a brute-force prefix maximum") {
    Rng rng;
    PrefixTree tree;
    std::map<std::int64_t, std::int64_t> model;
    for (int step = 0; step < 30'000; ++step) {
        INFO("step ", step);
        const std::int64_t key = rng.uniform(-200, 200);
        const auto op = rng.uniform(0, 3);
        if (op == 0 || op == 1) {
            const std::int64_t value = rng.uniform(-50, 50);
            const bool fresh = !model.contains(key);
            REQUIRE_EQ(tree.insert(key, value), fresh);
            model[key] = value;
        } else if (op == 2) {
            REQUIRE_EQ(tree.erase(key), model.erase(key) > 0);
        } else {
            const auto it = model.find(key);
            REQUIRE_EQ(tree.value_of(key), it == model.end() ? std::optional<std::int64_t>() : std::optional<std::int64_t>(it->second));
        }
        REQUIRE_EQ(tree.size(), model.size());
        if (step % 8 == 0) {
            check_max_prefix(tree, model);
        }
    }
}

PERF(1, "max_prefix is constant time on a large tree", 3000) {
    Rng rng;
    PrefixTree tree;
    for (int i = 0; i < 300'000; ++i) {
        tree.insert(i, rng.uniform(-1000, 1000));
    }
    // Rescanning all prefixes would be 3e5 steps per query here.
    std::int64_t sum = 0;
    for (int i = 0; i < 2'000'000; ++i) {
        sum += tree.max_prefix()->second;
    }
    for (int i = 0; i < 300'000; ++i) {
        tree.insert(rng.uniform(0, 299'999), rng.uniform(-1000, 1000));
        sum += tree.max_prefix()->second;
    }
    CHECK_NE(sum, 0);
}

TEST(2, "a hand-checked instance") {
    const std::vector<Topping> points{{1, 1, 5}, {2, 2, -3}, {3, 3, 4}};
    const auto got = tastiest_slice(points);
    REQUIRE(got.has_value());
    CHECK_EQ(*got, Slice{3, 3, 6});
}

TEST(2, "a prefix that skips a costly point") {
    // x-sorted: (10,3,+8), (20,1,-5), (30,2,+9). Taking y <= 3 includes the -5; taking y <= 2
    // cannot avoid it either, so compare the combinations by hand.
    const std::vector<Topping> points{{10, 3, 8}, {20, 1, -5}, {30, 2, 9}};
    check_slice(points);
    // Dropping the costly point by cutting at Y = 3 only for X = 10 yields 8; the whole sum is 12.
    CHECK_EQ(tastiest_slice(points)->t, 12);
}

TEST(2, "no positive slice returns nullopt") {
    const std::vector<Topping> points{{1, 2, -1}, {2, 1, -4}};
    CHECK(!tastiest_slice(points).has_value());
}

TEST(2, "all values zero is not positive") {
    const std::vector<Topping> points{{1, 1, 0}, {2, 2, 0}};
    CHECK(!tastiest_slice(points).has_value());
}

TEST(2, "negative coordinates and a single point") {
    const std::vector<Topping> one{{-5, -7, 3}};
    CHECK_EQ(*tastiest_slice(one), Slice{-5, -7, 3});
    const std::vector<Topping> points{{-1, 5, 4}, {0, -2, 6}, {7, 7, -9}};
    check_slice(points);
}

TEST(2, "empty input") {
    CHECK(!tastiest_slice(std::vector<Topping>{}).has_value());
}

TEST(2, "random distinct-coordinate instances match brute force") {
    Rng rng;
    for (int round = 0; round < 1000; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 18));
        std::vector<int> xs = rng.permutation(n);
        std::vector<int> ys = rng.permutation(n);
        std::vector<Topping> points;
        for (std::size_t i = 0; i < n; ++i) {
            points.push_back({xs[i] * 3 - 10, ys[i] * 7 - 20, rng.uniform(-30, 30)});
        }
        check_slice(points);
    }
}

PERF(2, "three hundred thousand distinct points", 3000) {
    Rng rng;
    const std::size_t n = 300'000;
    std::vector<int> xs = rng.permutation(n);
    std::vector<int> ys = rng.permutation(n);
    std::vector<Topping> points;
    points.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        points.push_back({xs[i], ys[i], rng.uniform(-1000, 1200)});
    }
    const auto got = tastiest_slice(points);
    CHECK(got.has_value());
}

TEST(3, "add inserts missing keys and accumulates on existing ones") {
    PrefixTree tree;
    tree.add(1, 5);
    CHECK_EQ(tree.value_of(1), std::optional<std::int64_t>(5));
    tree.add(1, -2);
    CHECK_EQ(tree.value_of(1), std::optional<std::int64_t>(3));
    tree.add(4, 10);
    CHECK_EQ(tree.size(), 2);
    CHECK_EQ(tree.max_prefix(), Answer(std::pair<std::int64_t, std::int64_t>{4, 13}));
}

TEST(3, "points sharing an x are all included before reading") {
    // Cutting at X = 2 must include both of its points, even though one is very negative.
    const std::vector<Topping> points{{2, 1, 10}, {2, 2, -100}, {3, 3, 1}};
    check_slice(points);
}

TEST(3, "points sharing a y accumulate on the same key") {
    const std::vector<Topping> points{{1, 5, 3}, {2, 5, 4}, {3, 5, -1}};
    const auto got = tastiest_slice(points);
    REQUIRE(got.has_value());
    CHECK_EQ(got->t, 7);
    CHECK_EQ(got->y, 5);
}

TEST(3, "a full grid of duplicates") {
    const std::vector<Topping> points{{1, 1, 2}, {1, 2, 2}, {2, 1, -3}, {2, 2, 9}, {3, 1, 1}};
    check_slice(points);
}

TEST(3, "random instances with heavy coordinate collisions match brute force") {
    Rng rng;
    for (int round = 0; round < 1500; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 25));
        std::vector<Topping> points;
        for (std::size_t i = 0; i < n; ++i) {
            points.push_back({rng.uniform(-3, 3), rng.uniform(-3, 3), rng.uniform(-15, 15)});
        }
        check_slice(points);
    }
}

// Many groups and many keys at once: rescanning the tree per x would take about 1e10 steps.
PERF(3, "many duplicates keep the same bound", 4000) {
    Rng rng;
    const std::size_t n = 400'000;
    std::vector<Topping> points;
    points.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        points.push_back({rng.uniform(0, 100'000), rng.uniform(0, 100'000), rng.uniform(-100, 120)});
    }
    const auto got = tastiest_slice(points);
    CHECK(got.has_value());
}
