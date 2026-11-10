#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;
using Set = SortedArraySet<int, std::string>;
using CountedSet = SortedArraySet<Counted<int>, int>;

namespace {

std::size_t search_budget(std::size_t n) {
    return 2 * static_cast<std::size_t>(std::ceil(std::log2(static_cast<double>(n) + 1.0))) + 2;
}

Set make_set(const std::vector<int>& keys) {
    std::vector<Set::Item> items;
    for (int key : keys) {
        items.emplace_back(key, "v" + std::to_string(key));
    }
    return Set(std::move(items));
}

std::vector<int> keys_of(const Set& set) {
    std::vector<int> out;
    for (const auto& item : set.items()) {
        out.push_back(item.first);
    }
    return out;
}

std::optional<int> key_of(const Set::Item* item) {
    return item == nullptr ? std::nullopt : std::optional<int>(item->first);
}

std::vector<Counted<int>> counted_values(std::size_t n) {
    std::vector<Counted<int>> values;
    values.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        values.emplace_back(static_cast<int>(2 * i));
    }
    return values;
}

CountedSet make_counted_set(std::size_t n) {
    std::vector<CountedSet::Item> items;
    items.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        items.emplace_back(Counted<int>(static_cast<int>(2 * i)), 0);
    }
    return CountedSet(std::move(items));
}

}  // namespace

TEST(1, "searching an empty span") {
    const std::vector<int> empty;
    CHECK_EQ(lower_bound_index<int>(empty, 5), 0);
    CHECK_EQ(upper_bound_index<int>(empty, 5), 0);
}

TEST(1, "key smaller and larger than everything") {
    const std::vector<int> values = {10, 20, 30};
    CHECK_EQ(lower_bound_index<int>(values, 5), 0);
    CHECK_EQ(upper_bound_index<int>(values, 5), 0);
    CHECK_EQ(lower_bound_index<int>(values, 35), 3);
    CHECK_EQ(upper_bound_index<int>(values, 35), 3);
}

TEST(1, "bounds around present and missing keys") {
    const std::vector<int> values = {10, 20, 30};
    CHECK_EQ(lower_bound_index<int>(values, 20), 1);
    CHECK_EQ(upper_bound_index<int>(values, 20), 2);
    CHECK_EQ(lower_bound_index<int>(values, 25), 2);
    CHECK_EQ(upper_bound_index<int>(values, 25), 2);
    CHECK_EQ(lower_bound_index<int>(values, 10), 0);
    CHECK_EQ(upper_bound_index<int>(values, 30), 3);
}

TEST(1, "runs of equal keys") {
    const std::vector<int> values = {1, 2, 2, 2, 2, 3};
    CHECK_EQ(lower_bound_index<int>(values, 2), 1);
    CHECK_EQ(upper_bound_index<int>(values, 2), 5);
    const std::vector<int> same(9, 4);
    CHECK_EQ(lower_bound_index<int>(same, 4), 0);
    CHECK_EQ(upper_bound_index<int>(same, 4), 9);
}

TEST(1, "random arrays match the standard library") {
    Rng rng;
    for (int round = 0; round < 3'000; ++round) {
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 40)), 0, 30);
        std::sort(values.begin(), values.end());
        for (int key = -1; key <= 31; ++key) {
            INFO("round ", round, " key ", key);
            const auto lower = std::lower_bound(values.begin(), values.end(), key) - values.begin();
            const auto upper = std::upper_bound(values.begin(), values.end(), key) - values.begin();
            REQUIRE_EQ(lower_bound_index<int>(values, key), lower);
            REQUIRE_EQ(upper_bound_index<int>(values, key), upper);
        }
    }
}

TEST(1, "comparison count is logarithmic for every size") {
    for (std::size_t n : {0U, 1U, 2U, 3U, 7U, 8U, 1'000U, 1'024U, 1'000'000U}) {
        const auto values = counted_values(n);
        const std::span<const Counted<int>> view(values);
        const int top = static_cast<int>(2 * n) + 1;
        for (int key : {-1, 0, 1, static_cast<int>(n), static_cast<int>(n) + 1, top}) {
            INFO("n=", n, " key=", key);
            Counted<int>::comparisons() = 0;
            lower_bound_index<Counted<int>>(view, Counted<int>(key));
            REQUIRE(Counted<int>::comparisons() <= search_budget(n));
            Counted<int>::comparisons() = 0;
            upper_bound_index<Counted<int>>(view, Counted<int>(key));
            REQUIRE(Counted<int>::comparisons() <= search_budget(n));
        }
    }
}

TEST(2, "a default-constructed set is empty") {
    Set set;
    CHECK_EQ(set.size(), 0);
    CHECK(set.items().empty());
    CHECK(set.find(1) == nullptr);
    CHECK(set.find_min() == nullptr);
    CHECK(set.find_max() == nullptr);
    CHECK(set.find_next(1) == nullptr);
    CHECK(set.find_prev(1) == nullptr);
}

TEST(2, "building sorts by key") {
    const auto set = make_set({30, 10, 20});
    CHECK_EQ(keys_of(set), std::vector<int>{10, 20, 30});
    CHECK_EQ(set.size(), 3);
}

TEST(2, "duplicate keys in build keep the last occurrence") {
    Set set({{1, "first"}, {2, "two"}, {1, "second"}, {1, "last"}});
    CHECK_EQ(keys_of(set), std::vector<int>{1, 2});
    REQUIRE(set.find(1) != nullptr);
    CHECK_EQ(set.find(1)->second, "last");
}

TEST(2, "find reports presence") {
    const auto set = make_set({1, 3, 5});
    REQUIRE(set.find(3) != nullptr);
    CHECK_EQ(set.find(3)->second, "v3");
    CHECK(set.find(4) == nullptr);
    CHECK(set.find(0) == nullptr);
    CHECK(set.find(6) == nullptr);
}

TEST(2, "find_min and find_max") {
    const auto set = make_set({8, 3, 9, 1});
    REQUIRE(set.find_min() != nullptr);
    REQUIRE(set.find_max() != nullptr);
    CHECK_EQ(set.find_min()->first, 1);
    CHECK_EQ(set.find_max()->first, 9);
}

TEST(2, "find_next is strictly greater") {
    const auto set = make_set({10, 20, 30});
    CHECK_EQ(key_of(set.find_next(10)), std::optional<int>(20));
    CHECK_EQ(key_of(set.find_next(15)), std::optional<int>(20));
    CHECK_EQ(key_of(set.find_next(5)), std::optional<int>(10));
    CHECK(set.find_next(30) == nullptr);
    CHECK(set.find_next(35) == nullptr);
}

TEST(2, "find_prev is strictly smaller") {
    const auto set = make_set({10, 20, 30});
    CHECK_EQ(key_of(set.find_prev(30)), std::optional<int>(20));
    CHECK_EQ(key_of(set.find_prev(25)), std::optional<int>(20));
    CHECK_EQ(key_of(set.find_prev(35)), std::optional<int>(30));
    CHECK(set.find_prev(10) == nullptr);
    CHECK(set.find_prev(5) == nullptr);
}

TEST(2, "queries on a single item") {
    const auto set = make_set({7});
    CHECK_EQ(key_of(set.find(7)), std::optional<int>(7));
    CHECK(set.find_next(7) == nullptr);
    CHECK(set.find_prev(7) == nullptr);
    CHECK_EQ(key_of(set.find_next(6)), std::optional<int>(7));
    CHECK_EQ(key_of(set.find_prev(8)), std::optional<int>(7));
}

TEST(2, "random sets match std::map") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        std::vector<Set::Item> items;
        std::map<int, std::string> model;
        const auto n = static_cast<std::size_t>(rng.uniform(0, 40));
        for (std::size_t i = 0; i < n; ++i) {
            const int key = static_cast<int>(rng.uniform(0, 60));
            const std::string value = "v" + std::to_string(i);
            items.emplace_back(key, value);
            model[key] = value;
        }
        Set set(items);
        INFO("round ", round);
        REQUIRE_EQ(set.size(), model.size());
        for (int key = -1; key <= 61; ++key) {
            const auto found = model.find(key);
            const auto next = model.upper_bound(key);
            auto prev = model.lower_bound(key);
            const std::optional<int> expected_prev =
                prev == model.begin() ? std::nullopt : std::optional<int>(std::prev(prev)->first);
            REQUIRE_EQ(key_of(set.find(key)),
                       found == model.end() ? std::nullopt : std::optional<int>(key));
            if (found != model.end()) {
                REQUIRE_EQ(set.find(key)->second, found->second);
            }
            REQUIRE_EQ(key_of(set.find_next(key)),
                       next == model.end() ? std::nullopt : std::optional<int>(next->first));
            REQUIRE_EQ(key_of(set.find_prev(key)), expected_prev);
        }
    }
}

TEST(2, "queries use a logarithmic number of comparisons") {
    for (std::size_t n : {1U, 2U, 17U, 1'000U, 200'000U}) {
        const auto set = make_counted_set(n);
        const std::size_t budget = search_budget(n) + 4;
        for (const int key : {-5, 0, 7, 1'001, static_cast<int>(n) * 2 + 1}) {
            INFO("n=", n, " key=", key);
            const Counted<int> query(key);
            Counted<int>::comparisons() = 0;
            (void)set.find(query);
            REQUIRE(Counted<int>::comparisons() <= budget);
            Counted<int>::comparisons() = 0;
            (void)set.find_next(query);
            REQUIRE(Counted<int>::comparisons() <= budget);
            Counted<int>::comparisons() = 0;
            (void)set.find_prev(query);
            REQUIRE(Counted<int>::comparisons() <= budget);
        }
    }
}

TEST(2, "build uses O(n log n) comparisons") {
    Rng rng;
    const std::size_t n = 50'000;
    std::vector<CountedSet::Item> items;
    items.reserve(n);
    for (const int key : rng.ints(n, 0, 1'000'000)) {
        items.emplace_back(Counted<int>(key), 0);
    }
    Counted<int>::comparisons() = 0;
    const CountedSet set(std::move(items));
    const double budget = 4.0 * static_cast<double>(n) * std::log2(static_cast<double>(n));
    CHECK_LE(static_cast<double>(Counted<int>::comparisons()), budget);
    CHECK_GT(set.size(), 0);
}

TEST(3, "insert adds new keys in sorted position") {
    Set set;
    CHECK(set.insert({5, "five"}));
    CHECK(set.insert({1, "one"}));
    CHECK(set.insert({9, "nine"}));
    CHECK_EQ(keys_of(set), std::vector<int>{1, 5, 9});
}

TEST(3, "insert of an existing key replaces the value") {
    auto set = make_set({1, 2});
    CHECK(!set.insert({2, "replaced"}));
    CHECK_EQ(set.size(), 2);
    CHECK_EQ(set.find(2)->second, "replaced");
}

TEST(3, "erase returns the removed item") {
    auto set = make_set({1, 2, 3});
    const auto removed = set.erase(2);
    REQUIRE(removed.has_value());
    CHECK_EQ(removed->first, 2);
    CHECK_EQ(removed->second, "v2");
    CHECK_EQ(keys_of(set), std::vector<int>{1, 3});
}

TEST(3, "erasing a missing key returns nullopt") {
    auto set = make_set({1, 3});
    CHECK(!set.erase(2).has_value());
    CHECK_EQ(set.size(), 2);
    Set empty;
    CHECK(!empty.erase(1).has_value());
}

TEST(3, "random insertions and erasures match std::map") {
    Rng rng;
    Set set;
    std::map<int, std::string> model;
    for (int step = 0; step < 6'000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 80));
        if (rng.coin(0.55)) {
            const std::string value = "s" + std::to_string(step);
            const bool inserted = !model.contains(key);
            REQUIRE_EQ(set.insert({key, value}), inserted);
            model[key] = value;
        } else {
            const auto it = model.find(key);
            const auto removed = set.erase(key);
            REQUIRE_EQ(removed.has_value(), it != model.end());
            if (it != model.end()) {
                REQUIRE_EQ(removed->second, it->second);
                model.erase(it);
            }
        }
        REQUIRE_EQ(set.size(), model.size());
        const auto next = model.upper_bound(key);
        REQUIRE_EQ(key_of(set.find_next(key)),
                   next == model.end() ? std::nullopt : std::optional<int>(next->first));
    }
    std::vector<int> expected;
    for (const auto& [key, value] : model) {
        expected.push_back(key);
    }
    CHECK_EQ(keys_of(set), expected);
}

TEST(3, "insert and erase find their position with a logarithmic number of comparisons") {
    const std::size_t n = 3'000;
    auto set = make_counted_set(n);
    const std::size_t budget = search_budget(n + 1) + 6;
    Rng rng;
    for (int step = 0; step < 300; ++step) {
        const int odd_key = static_cast<int>(2 * rng.uniform(0, static_cast<std::int64_t>(n) - 1) + 1);
        INFO("step ", step, " key=", odd_key);
        Counted<int>::comparisons() = 0;
        set.insert({Counted<int>(odd_key), step});
        REQUIRE(Counted<int>::comparisons() <= budget);
        Counted<int>::comparisons() = 0;
        set.erase(Counted<int>(odd_key));
        REQUIRE(Counted<int>::comparisons() <= budget);
    }
}

TEST(4, "range returns the half-open interval") {
    const auto set = make_set({10, 20, 30, 40});
    const auto view = set.range(20, 40);
    REQUIRE_EQ(view.size(), 2);
    CHECK_EQ(view[0].first, 20);
    CHECK_EQ(view[1].first, 30);
    CHECK_EQ(set.range(15, 35).size(), 2);
    CHECK_EQ(set.range(0, 100).size(), 4);
}

TEST(4, "range edge cases") {
    const auto set = make_set({10, 20});
    CHECK(set.range(20, 20).empty());
    CHECK(set.range(21, 25).empty());
    CHECK(set.range(5, 9).empty());
    CHECK(set.range(30, 5).empty());
    CHECK(Set{}.range(0, 10).empty());
}

TEST(4, "range is a zero-copy view of the set") {
    const auto set = make_set({1, 2, 3, 4});
    const auto view = set.range(2, 4);
    CHECK(view.data() == set.items().data() + 1);
}

TEST(4, "as_of finds the latest item at or before t") {
    const auto set = make_set({10, 20, 30});
    CHECK_EQ(key_of(set.as_of(20)), std::optional<int>(20));
    CHECK_EQ(key_of(set.as_of(25)), std::optional<int>(20));
    CHECK_EQ(key_of(set.as_of(100)), std::optional<int>(30));
    CHECK_EQ(key_of(set.as_of(10)), std::optional<int>(10));
    CHECK(set.as_of(9) == nullptr);
    CHECK(Set{}.as_of(0) == nullptr);
}

TEST(4, "random range and as_of queries match std::map") {
    Rng rng;
    for (int round = 0; round < 800; ++round) {
        std::map<int, std::string> model;
        const auto n = static_cast<std::size_t>(rng.uniform(0, 50));
        for (std::size_t i = 0; i < n; ++i) {
            model[static_cast<int>(rng.uniform(0, 100))] = "x";
        }
        std::vector<Set::Item> items(model.begin(), model.end());
        rng.shuffle(items);
        const Set set(items);
        for (int query = 0; query < 30; ++query) {
            const int a = static_cast<int>(rng.uniform(-5, 105));
            const int b = static_cast<int>(rng.uniform(-5, 105));
            INFO("round ", round, " a=", a, " b=", b);
            std::size_t expected = 0;
            for (const auto& [key, value] : model) {
                expected += (key >= a && key < b) ? 1 : 0;
            }
            REQUIRE_EQ(set.range(a, b).size(), b < a ? 0 : expected);
            auto last = model.upper_bound(a);
            const std::optional<int> expected_as_of =
                last == model.begin() ? std::nullopt : std::optional<int>(std::prev(last)->first);
            REQUIRE_EQ(key_of(set.as_of(a)), expected_as_of);
        }
    }
}

TEST(4, "range and as_of are logarithmic in comparisons") {
    const std::size_t n = 100'000;
    const auto set = make_counted_set(n);
    const std::size_t budget = 2 * search_budget(n) + 6;
    Counted<int>::comparisons() = 0;
    CHECK_GT(set.range(Counted<int>(10), Counted<int>(50'000)).size(), 0);
    CHECK_LE(Counted<int>::comparisons(), budget);
    Counted<int>::comparisons() = 0;
    CHECK(set.as_of(Counted<int>(123'456)) != nullptr);
    CHECK_LE(Counted<int>::comparisons(), budget);
}

// Linear searches would make these million queries quadratic.
PERF(2, "a million queries over a million keys", 1200) {
    std::vector<SortedArraySet<int, int>::Item> items;
    items.reserve(1'000'000);
    for (int i = 0; i < 1'000'000; ++i) {
        items.emplace_back(2 * i, i);
    }
    const SortedArraySet<int, int> set(std::move(items));
    Rng rng;
    long long hits = 0;
    for (int i = 0; i < 1'000'000; ++i) {
        const int key = static_cast<int>(rng.uniform(0, 2'000'000));
        hits += set.find(key) != nullptr ? 1 : 0;
        hits += set.find_next(key) != nullptr ? 1 : 0;
    }
    CHECK_GT(hits, 0);
}
