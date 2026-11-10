#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;
using practice::Tracked;

namespace {

struct TrackedKey {
    explicit TrackedKey(int v) : value(v) {}
    Tracked value;
    friend bool operator<(const TrackedKey& a, const TrackedKey& b) {
        return a.value.value() < b.value.value();
    }
};

template <typename Key>
std::vector<Key> level0(const SkipList<Key>& list) {
    std::vector<Key> keys;
    for (const auto* node = list.first(); node != nullptr; node = node->next[0]) {
        keys.push_back(node->key);
    }
    return keys;
}

// Every level is sorted and every node in a level also appears in the level below.
template <typename Key>
void check_structure(const SkipList<Key>& list) {
    using Node = typename SkipList<Key>::Node;
    std::vector<std::set<const Node*>> levels(static_cast<std::size_t>(list.height()));
    for (int level = 0; level < list.height(); ++level) {
        const Node* previous = nullptr;
        for (const Node* node = list.first_at(level); node != nullptr;
             node = node->next[static_cast<std::size_t>(level)]) {
            CHECK_GT(node->next.size(), static_cast<std::size_t>(level));
            if (previous != nullptr) {
                CHECK(previous->key < node->key);
            }
            previous = node;
            levels[static_cast<std::size_t>(level)].insert(node);
        }
        if (level > 0) {
            for (const Node* node : levels[static_cast<std::size_t>(level)]) {
                CHECK(levels[static_cast<std::size_t>(level) - 1].contains(node));
            }
        }
    }
    if (!levels.empty()) {
        CHECK_EQ(levels[0].size(), list.size());
    } else {
        CHECK_EQ(list.size(), 0);
    }
}

}  // namespace

TEST(1, "a new list is empty") {
    const SkipList<int> list;
    CHECK(list.empty());
    CHECK_EQ(list.size(), 0);
    CHECK_EQ(list.height(), 0);
    CHECK(list.first() == nullptr);
    CHECK(list.find(1) == nullptr);
    CHECK(!list.contains(1));
}

TEST(1, "inserted keys can be found in order") {
    SkipList<int> list;
    for (int key : {5, 1, 9, 3}) {
        CHECK(list.insert(key));
    }
    CHECK_EQ(level0(list), (std::vector<int>{1, 3, 5, 9}));
    CHECK(list.contains(3));
    CHECK(!list.contains(4));
    const auto* node = list.find(9);
    REQUIRE(node != nullptr);
    CHECK_EQ(node->key, 9);
}

TEST(1, "inserting a duplicate is refused") {
    SkipList<int> list;
    CHECK(list.insert(1));
    CHECK(!list.insert(1));
    CHECK_EQ(list.size(), 1);
    check_structure(list);
}

TEST(1, "the structure is valid for adversarial insertion orders") {
    for (bool ascending : {true, false}) {
        SkipList<int> list;
        for (int i = 0; i < 500; ++i) {
            list.insert(ascending ? i : 500 - i);
        }
        check_structure(list);
    }
}

TEST(1, "a given seed produces the same structure") {
    SkipList<int> a(7);
    SkipList<int> b(7);
    for (int key = 0; key < 200; ++key) {
        a.insert(key);
        b.insert(key);
    }
    CHECK_EQ(a.height(), b.height());
    for (int key = 0; key < 200; ++key) {
        CHECK_EQ(a.find(key)->next.size(), b.find(key)->next.size());
    }
}

TEST(1, "moving transfers ownership and leaves the source usable") {
    SkipList<int> source;
    source.insert(1);
    SkipList<int> moved(std::move(source));
    CHECK_EQ(moved.size(), 1);
    CHECK(moved.contains(1));
    CHECK_EQ(source.size(), 0);
    CHECK(source.first() == nullptr);
    source.insert(2);
    CHECK(source.contains(2));
}

TEST(1, "move assignment replaces the old contents") {
    SkipList<int> a;
    SkipList<int> b;
    a.insert(1);
    b.insert(2);
    a = std::move(b);
    CHECK(a.contains(2));
    CHECK(!a.contains(1));
}

TEST(1, "no keys leak after destruction") {
    const auto before = Tracked::live();
    {
        SkipList<TrackedKey> list;
        for (int i = 0; i < 1000; ++i) {
            list.insert(TrackedKey(i));
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "destroying a very long list does not overflow the stack") {
    SkipList<int> list;
    for (int i = 0; i < 300'000; ++i) {
        list.insert(i);
    }
    CHECK_EQ(list.size(), 300'000);
}

TEST(1, "random inserts match std::set") {
    Rng rng;
    SkipList<int> list(static_cast<std::uint64_t>(rng.uniform(1, 1'000'000)));
    std::set<int> model;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 8000));
        REQUIRE_EQ(list.insert(key), model.insert(key).second);
        const int probe = static_cast<int>(rng.uniform(0, 8000));
        REQUIRE_EQ(list.contains(probe), model.contains(probe));
    }
    CHECK_EQ(level0(list), std::vector<int>(model.begin(), model.end()));
    check_structure(list);
}

TEST(1, "the height stays logarithmic") {
    Rng rng;
    SkipList<int> list(99);
    const int n = 1 << 16;
    for (int key : rng.permutation(n)) {
        list.insert(key);
    }
    CHECK_LE(list.height(), 3 * 16);
    CHECK_GE(list.height(), 8);
}

TEST(1, "searches cost O(log n) comparisons on average") {
    Rng rng;
    SkipList<Counted<int>> list(4242);
    const int n = 1 << 16;
    for (int key : rng.permutation(n)) {
        list.insert(Counted<int>(key));
    }
    Counted<int>::comparisons() = 0;
    const int queries = 5000;
    int found = 0;
    for (int i = 0; i < queries; ++i) {
        found += list.contains(Counted<int>(static_cast<int>(rng.uniform(0, n - 1))));
    }
    CHECK_EQ(found, queries);
    CHECK_LE(static_cast<double>(Counted<int>::comparisons()) / queries, 5.0 * 16);
}

PERF(1, "half a million inserts and lookups", 5000) {
    Rng rng;
    SkipList<int> list(1234);
    const auto keys = rng.permutation(500'000);
    for (int key : keys) {
        list.insert(key);
    }
    int found = 0;
    for (int key : keys) {
        found += list.contains(key);
    }
    CHECK_EQ(found, 500'000);
}

TEST(2, "erasing a missing key changes nothing") {
    SkipList<int> list;
    list.insert(1);
    CHECK(!list.erase(2));
    CHECK_EQ(list.size(), 1);
    check_structure(list);
}

TEST(2, "erasing from an empty list reports absence") {
    SkipList<int> list;
    CHECK(!list.erase(1));
}

TEST(2, "erasing the only key empties the list") {
    SkipList<int> list;
    list.insert(1);
    CHECK(list.erase(1));
    CHECK(list.empty());
    CHECK_EQ(list.height(), 0);
    CHECK(list.first() == nullptr);
    CHECK(list.insert(1));
}

TEST(2, "erased keys are unlinked from every level") {
    SkipList<int> list(11);
    for (int key = 0; key < 200; ++key) {
        list.insert(key);
    }
    for (int key = 0; key < 200; key += 3) {
        CHECK(list.erase(key));
    }
    check_structure(list);
    CHECK(!list.contains(0));
    CHECK(list.contains(1));
}

TEST(2, "the height shrinks when the tallest towers disappear") {
    SkipList<int> list(5);
    for (int key = 0; key < 1000; ++key) {
        list.insert(key);
    }
    for (int key = 0; key < 1000; ++key) {
        CHECK(list.erase(key));
    }
    CHECK(list.empty());
    CHECK_EQ(list.height(), 0);
}

TEST(2, "no keys leak through deletions") {
    const auto before = Tracked::live();
    {
        SkipList<TrackedKey> list;
        for (int i = 0; i < 500; ++i) {
            list.insert(TrackedKey(i));
        }
        for (int i = 0; i < 500; i += 2) {
            list.erase(TrackedKey(i));
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(2, "random operations match std::set") {
    Rng rng;
    SkipList<int> list(321);
    std::set<int> model;
    for (int step = 0; step < 30'000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 3000));
        if (rng.coin(0.55)) {
            REQUIRE_EQ(list.insert(key), model.insert(key).second);
        } else {
            REQUIRE_EQ(list.erase(key), model.erase(key) == 1);
        }
        REQUIRE_EQ(list.size(), model.size());
        const int probe = static_cast<int>(rng.uniform(0, 3000));
        REQUIRE_EQ(list.contains(probe), model.contains(probe));
    }
    CHECK_EQ(level0(list), std::vector<int>(model.begin(), model.end()));
    check_structure(list);
}

PERF(2, "mixed updates at scale", 4000) {
    Rng rng;
    SkipList<int> list(777);
    const auto keys = rng.permutation(400'000);
    for (int key : keys) {
        list.insert(key);
    }
    for (int key : keys) {
        CHECK(list.erase(key));
    }
    CHECK(list.empty());
}

TEST(3, "a new scheduler has nothing pending") {
    JobScheduler scheduler;
    CHECK_EQ(scheduler.pending(), 0);
    CHECK(scheduler.pop_due(100).empty());
}

TEST(3, "due jobs come out earliest first") {
    JobScheduler scheduler;
    const int a = scheduler.schedule(30);
    const int b = scheduler.schedule(10);
    const int c = scheduler.schedule(20);
    CHECK_EQ(scheduler.pending(), 3);
    CHECK_EQ(scheduler.pop_due(20), (std::vector<int>{b, c}));
    CHECK_EQ(scheduler.pending(), 1);
    CHECK_EQ(scheduler.pop_due(100), std::vector<int>{a});
    CHECK_EQ(scheduler.pending(), 0);
}

TEST(3, "ties at the same time come out in scheduling order") {
    JobScheduler scheduler;
    const int a = scheduler.schedule(5);
    const int b = scheduler.schedule(5);
    const int c = scheduler.schedule(5);
    CHECK_EQ(scheduler.pop_due(5), (std::vector<int>{a, b, c}));
}

TEST(3, "cancelled jobs do not run") {
    JobScheduler scheduler;
    const int a = scheduler.schedule(5);
    const int b = scheduler.schedule(6);
    CHECK(scheduler.cancel(a));
    CHECK_EQ(scheduler.pending(), 1);
    CHECK_EQ(scheduler.pop_due(10), std::vector<int>{b});
}

TEST(3, "cancelling an unknown or finished job reports false") {
    JobScheduler scheduler;
    CHECK(!scheduler.cancel(42));
    const int a = scheduler.schedule(1);
    scheduler.pop_due(1);
    CHECK(!scheduler.cancel(a));
    CHECK(!scheduler.cancel(a));
}

TEST(3, "random operations match a multimap of pending jobs") {
    Rng rng;
    JobScheduler scheduler(55);
    std::map<std::pair<std::int64_t, int>, bool> model;
    std::map<int, std::int64_t> when;
    std::int64_t now = 0;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const auto action = rng.uniform(0, 2);
        if (action == 0) {
            const std::int64_t at = now + rng.uniform(0, 100);
            const int job = scheduler.schedule(at);
            model[{at, job}] = true;
            when[job] = at;
        } else if (action == 1 && !when.empty()) {
            const int job = std::next(when.begin(), rng.uniform(0, static_cast<std::int64_t>(when.size()) - 1))->first;
            REQUIRE(scheduler.cancel(job));
            model.erase({when[job], job});
            when.erase(job);
        } else {
            now += rng.uniform(0, 5);
            std::vector<int> expected;
            while (!model.empty() && model.begin()->first.first <= now) {
                expected.push_back(model.begin()->first.second);
                when.erase(model.begin()->first.second);
                model.erase(model.begin());
            }
            REQUIRE_EQ(scheduler.pop_due(now), expected);
        }
        REQUIRE_EQ(scheduler.pending(), model.size());
    }
}

PERF(3, "scheduling and draining half a million jobs", 5000) {
    Rng rng;
    JobScheduler scheduler(9);
    for (int i = 0; i < 500'000; ++i) {
        scheduler.schedule(rng.uniform(0, 1'000'000'000));
    }
    std::size_t drained = 0;
    for (std::int64_t now = 1'000; drained < 500'000; now *= 2) {
        drained += scheduler.pop_due(now).size();
    }
    CHECK_EQ(drained, 500'000);
}
