#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;

namespace {

using Map = AvlMap<int, int>;
using Set = AvlSet<int>;

template <typename Node>
void check_keys_sorted(const std::vector<int>& keys, const Node* root, std::size_t size) {
    REQUIRE_EQ(keys.size(), size);
    if (root == nullptr) {
        REQUIRE_EQ(size, 0);
        return;
    }
    REQUIRE(root->parent == nullptr);
    REQUIRE(root->size == size);
    REQUIRE(root->height <= 1.45 * std::log2(static_cast<double>(size) + 2));
}

std::vector<int> keys_of(const Map& map) {
    std::vector<int> keys;
    for (const auto& item : map.tree()) {
        keys.push_back(item.key);
    }
    return keys;
}

std::vector<int> keys_of(const Set& set) {
    std::vector<int> keys;
    for (const auto& item : set.tree()) {
        keys.push_back(item.key);
    }
    return keys;
}

}  // namespace

TEST(1, "a new map is empty") {
    Map map;
    CHECK(map.empty());
    CHECK_EQ(map.size(), 0);
    CHECK(map.find(1) == nullptr);
    CHECK(map.min() == nullptr);
    CHECK(map.max() == nullptr);
    CHECK(!map.erase(1));
}

TEST(1, "insert, find and overwrite") {
    Map map;
    CHECK(map.insert(5, 50));
    CHECK(map.insert(2, 20));
    CHECK(!map.insert(5, 55));
    CHECK_EQ(map.size(), 2);
    REQUIRE(map.find(5) != nullptr);
    CHECK_EQ(map.find(5)->item.value, 55);
    CHECK_EQ(map.find(2)->item.key, 2);
    CHECK(map.contains(2));
    CHECK(!map.contains(3));
}

TEST(1, "min and max follow the keys") {
    Map map;
    for (int key : {4, 1, 9, 3}) {
        map.insert(key, key * 10);
    }
    REQUIRE(map.min() != nullptr);
    CHECK_EQ(map.min()->item.key, 1);
    CHECK_EQ(map.max()->item.key, 9);
    CHECK(map.erase(1));
    CHECK(map.erase(9));
    CHECK_EQ(map.min()->item.key, 3);
    CHECK_EQ(map.max()->item.key, 4);
}

TEST(1, "set_value changes the value and is visible through find") {
    Map map;
    map.insert(1, 10);
    map.set_value(map.find(1), 99);
    CHECK_EQ(map.find(1)->item.value, 99);
}

TEST(1, "a custom comparator orders the keys") {
    AvlMap<int, int, NoAugment, std::greater<int>> map{std::greater<int>{}};
    map.insert(1, 1);
    map.insert(3, 3);
    map.insert(2, 2);
    std::vector<int> keys;
    for (const auto& item : map.tree()) {
        keys.push_back(item.key);
    }
    CHECK_EQ(keys, std::vector<int>{3, 2, 1});
    CHECK_EQ(map.min()->item.key, 3);
}

TEST(1, "the AvlSet alias stores keys only") {
    Set set;
    CHECK(set.insert(7));
    CHECK(!set.insert(7));
    CHECK(set.contains(7));
    CHECK(set.erase(7));
    CHECK(!set.contains(7));
}

TEST(1, "every entry is released") {
    const auto before = Tracked::live();
    {
        AvlMap<int, Tracked> map;
        for (int i = 0; i < 1000; ++i) {
            map.insert(i, Tracked(i));
        }
        for (int i = 0; i < 400; ++i) {
            map.erase(i * 2);
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "random operations match std::map and stay balanced") {
    Rng rng;
    Map map;
    std::map<int, int> model;
    for (int step = 0; step < 40'000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 3000));
        const auto op = rng.uniform(0, 3);
        if (op <= 1) {
            const int value = static_cast<int>(rng.uniform(0, 1'000'000));
            REQUIRE_EQ(map.insert(key, value), model.insert_or_assign(key, value).second);
        } else if (op == 2) {
            REQUIRE_EQ(map.erase(key), model.erase(key) > 0);
        } else {
            const auto it = model.find(key);
            auto* node = map.find(key);
            REQUIRE_EQ(node != nullptr, it != model.end());
            if (node != nullptr) {
                REQUIRE_EQ(node->item.value, it->second);
            }
        }
        REQUIRE_EQ(map.size(), model.size());
        if (step % 100 == 0) {
            std::vector<int> expected;
            for (const auto& [k, v] : model) {
                expected.push_back(k);
            }
            REQUIRE_EQ(keys_of(map), expected);
            check_keys_sorted(expected, map.tree().root(), model.size());
        }
    }
}

PERF(1, "300k random keys inserted, found and erased", 4000) {
    Rng rng;
    Map map;
    std::vector<int> keys(300'000);
    for (int& key : keys) {
        key = static_cast<int>(rng.uniform(0, 2'000'000'000));
    }
    for (int key : keys) {
        map.insert(key, key);
    }
    for (int key : keys) {
        REQUIRE(map.contains(key));
    }
    for (int key : keys) {
        map.erase(key);
    }
    CHECK(map.empty());
}

TEST(2, "find_next and find_prev with present and absent keys") {
    Set set;
    for (int key : {10, 20, 30, 40}) {
        set.insert(key);
    }
    CHECK_EQ(set.find_next(10)->item.key, 20);
    CHECK_EQ(set.find_next(15)->item.key, 20);
    CHECK_EQ(set.find_next(5)->item.key, 10);
    CHECK(set.find_next(40) == nullptr);
    CHECK(set.find_next(99) == nullptr);
    CHECK(set.find_prev(10) == nullptr);
    CHECK_EQ(set.find_prev(25)->item.key, 20);
    CHECK_EQ(set.find_prev(40)->item.key, 30);
    CHECK_EQ(set.find_prev(100)->item.key, 40);
}

TEST(2, "an empty set answers null") {
    Set set;
    CHECK(set.find_next(1) == nullptr);
    CHECK(set.find_prev(1) == nullptr);
}

TEST(2, "random queries match std::set") {
    Rng rng;
    Set set;
    std::set<int> model;
    for (int step = 0; step < 40'000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 800));
        const auto op = rng.uniform(0, 3);
        if (op == 0) {
            REQUIRE_EQ(set.insert(key), model.insert(key).second);
        } else if (op == 1) {
            REQUIRE_EQ(set.erase(key), model.erase(key) > 0);
        } else if (op == 2) {
            const auto it = model.upper_bound(key);
            auto* node = set.find_next(key);
            REQUIRE_EQ(node != nullptr, it != model.end());
            if (node != nullptr) {
                REQUIRE_EQ(node->item.key, *it);
            }
        } else {
            const auto it = model.lower_bound(key);
            auto* node = set.find_prev(key);
            REQUIRE_EQ(node != nullptr, it != model.begin());
            if (node != nullptr) {
                REQUIRE_EQ(node->item.key, *std::prev(it));
            }
        }
    }
}

TEST(3, "rank counts strictly smaller keys") {
    Set set;
    for (int key : {10, 20, 30, 40}) {
        set.insert(key);
    }
    CHECK_EQ(set.rank(5), 0);
    CHECK_EQ(set.rank(10), 0);
    CHECK_EQ(set.rank(11), 1);
    CHECK_EQ(set.rank(40), 3);
    CHECK_EQ(set.rank(41), 4);
}

TEST(3, "select finds the i-th smallest and rejects bad indices") {
    Set set;
    for (int key : {30, 10, 20}) {
        set.insert(key);
    }
    CHECK_EQ(set.select(0)->item.key, 10);
    CHECK_EQ(set.select(1)->item.key, 20);
    CHECK_EQ(set.select(2)->item.key, 30);
    CHECK_THROWS_AS(set.select(3), std::out_of_range);
    Set none;
    CHECK_THROWS_AS(none.select(0), std::out_of_range);
    CHECK_EQ(none.rank(5), 0);
}

TEST(3, "rank and select are inverses and match a sorted model") {
    Rng rng;
    Set set;
    std::set<int> model;
    for (int step = 0; step < 15'000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 2000));
        if (rng.coin(0.6)) {
            REQUIRE_EQ(set.insert(key), model.insert(key).second);
        } else {
            REQUIRE_EQ(set.erase(key), model.erase(key) > 0);
        }
        if (step % 16 == 0 && !model.empty()) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            const int probe = static_cast<int>(rng.uniform(-10, 2010));
            const auto it = model.lower_bound(probe);
            REQUIRE_EQ(set.rank(probe), static_cast<std::size_t>(std::distance(model.begin(), it)));
            auto expected = model.begin();
            std::advance(expected, static_cast<std::ptrdiff_t>(i));
            REQUIRE_EQ(set.select(i)->item.key, *expected);
            REQUIRE_EQ(set.rank(*expected), i);
        }
    }
}

TEST(4, "scores sort high to low with name ties broken ascending") {
    Leaderboard board;
    board.upsert("carol", 10);
    board.upsert("alice", 20);
    board.upsert("bob", 20);
    board.upsert("dan", -5);
    CHECK_EQ(board.size(), 4);
    CHECK_EQ(board.rank_of("alice"), std::optional<std::size_t>(0));
    CHECK_EQ(board.rank_of("bob"), std::optional<std::size_t>(1));
    CHECK_EQ(board.rank_of("carol"), std::optional<std::size_t>(2));
    CHECK_EQ(board.rank_of("dan"), std::optional<std::size_t>(3));
    CHECK_EQ(board.rank_of("nobody"), std::optional<std::size_t>());
    CHECK_EQ(board.player_at(0), std::optional<std::string>("alice"));
    CHECK_EQ(board.player_at(4), std::optional<std::string>());
}

TEST(4, "upsert moves a player and remove deletes one") {
    Leaderboard board;
    board.upsert("a", 1);
    board.upsert("b", 2);
    board.upsert("a", 3);
    CHECK_EQ(board.size(), 2);
    CHECK_EQ(board.player_at(0), std::optional<std::string>("a"));
    CHECK(board.remove("a"));
    CHECK(!board.remove("a"));
    CHECK_EQ(board.player_at(0), std::optional<std::string>("b"));
}

TEST(4, "top returns the leaders, capped at the player count") {
    Leaderboard board;
    board.upsert("x", 1);
    board.upsert("y", 3);
    board.upsert("z", 2);
    using Row = std::pair<std::string, std::int64_t>;
    CHECK_EQ(board.top(2), std::vector<Row>{{"y", 3}, {"z", 2}});
    CHECK_EQ(board.top(10), std::vector<Row>{{"y", 3}, {"z", 2}, {"x", 1}});
    CHECK(board.top(0).empty());
}

TEST(4, "random traffic matches a brute-force board") {
    Rng rng;
    Leaderboard board;
    std::map<std::string, std::int64_t> model;
    const auto expected_rows = [&] {
        std::vector<std::pair<std::string, std::int64_t>> rows(model.begin(), model.end());
        std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
            return a.second != b.second ? a.second > b.second : a.first < b.first;
        });
        return rows;
    };
    for (int step = 0; step < 6000; ++step) {
        INFO("step ", step);
        const std::string player = "p" + std::to_string(rng.uniform(0, 80));
        if (rng.coin(0.7)) {
            const std::int64_t score = rng.uniform(-30, 30);
            board.upsert(player, score);
            model[player] = score;
        } else {
            REQUIRE_EQ(board.remove(player), model.erase(player) > 0);
        }
        REQUIRE_EQ(board.size(), model.size());
        if (step % 25 == 0) {
            const auto rows = expected_rows();
            REQUIRE_EQ(board.top(rows.size() + 3), rows);
            for (std::size_t i = 0; i < rows.size(); ++i) {
                REQUIRE_EQ(board.rank_of(rows[i].first), std::optional<std::size_t>(i));
                REQUIRE_EQ(board.player_at(i), std::optional<std::string>(rows[i].first));
            }
        }
    }
}

PERF(4, "a large board under heavy update traffic", 4000) {
    Rng rng;
    Leaderboard board;
    const int players = 200'000;
    for (int i = 0; i < players; ++i) {
        board.upsert("player" + std::to_string(i), rng.uniform(0, 1'000'000));
    }
    std::uint64_t sum = 0;
    for (int i = 0; i < 400'000; ++i) {
        const std::string player = "player" + std::to_string(rng.uniform(0, players - 1));
        if (i % 2 == 0) {
            board.upsert(player, rng.uniform(0, 1'000'000));
        } else {
            sum += *board.rank_of(player);
        }
    }
    CHECK_EQ(board.top(100).size(), 100);
    CHECK_GT(sum, 0);
}
