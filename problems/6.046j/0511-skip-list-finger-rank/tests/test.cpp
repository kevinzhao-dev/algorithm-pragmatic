#include <cmath>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;
using practice::Tracked;
using List = IndexedSkipList<int>;

namespace {

struct TrackedKey {
    explicit TrackedKey(int v) : value(v) {}
    Tracked value;
    friend bool operator<(const TrackedKey& a, const TrackedKey& b) {
        return a.value.value() < b.value.value();
    }
};

void check_order(const List& list, const std::vector<int>& expected) {
    REQUIRE_EQ(list.size(), expected.size());
    std::vector<int> forward;
    const List::Node* last = nullptr;
    for (const List::Node* node = list.first(); node != nullptr; node = list.next(node)) {
        forward.push_back(node->key());
        last = node;
    }
    CHECK_EQ(forward, expected);
    std::vector<int> backward;
    for (const List::Node* node = last; node != nullptr; node = list.prev(node)) {
        backward.push_back(node->key());
    }
    std::ranges::reverse(backward);
    CHECK_EQ(backward, expected);
}

std::vector<List::Node*> filled(List& list, int n) {
    std::vector<List::Node*> nodes(static_cast<std::size_t>(n));
    Rng rng;
    for (int key : rng.permutation(static_cast<std::size_t>(n))) {
        nodes[static_cast<std::size_t>(key)] = list.insert(key);
    }
    return nodes;
}

double average_cost_limit(int distance) {
    return 6.0 * (std::log2(distance + 1.0) + 2.0);
}

}  // namespace

TEST(1, "a new list is empty") {
    const List list;
    CHECK(list.empty());
    CHECK_EQ(list.size(), 0);
    CHECK(list.first() == nullptr);
    CHECK(list.find(1) == nullptr);
}

TEST(1, "insert returns the node and refuses duplicates") {
    List list;
    const List::Node* node = list.insert(5);
    REQUIRE(node != nullptr);
    CHECK_EQ(node->key(), 5);
    CHECK(list.insert(5) == nullptr);
    CHECK_EQ(list.size(), 1);
}

TEST(1, "neighbors walk the keys in both directions") {
    List list;
    for (int key : {4, 2, 8, 6}) {
        list.insert(key);
    }
    check_order(list, {2, 4, 6, 8});
}

TEST(1, "next and prev stop at the ends") {
    List list;
    const auto* only = list.insert(1);
    CHECK(list.next(only) == nullptr);
    CHECK(list.prev(only) == nullptr);
}

TEST(1, "find returns the matching node") {
    List list;
    const auto* three = list.insert(3);
    list.insert(9);
    CHECK(list.find(3) == three);
    CHECK(list.find(4) == nullptr);
}

TEST(1, "nodes stay valid while other keys come and go") {
    List list;
    auto* keep = list.insert(50);
    for (int key = 0; key < 200; ++key) {
        if (key != 50) {
            list.insert(key);
        }
    }
    for (int key = 0; key < 200; key += 2) {
        if (key != 50) {
            list.erase(key);
        }
    }
    CHECK_EQ(keep->key(), 50);
    CHECK(list.find(50) == keep);
}

TEST(1, "erase reports whether a key was removed") {
    List list;
    list.insert(1);
    list.insert(2);
    CHECK(!list.erase(3));
    CHECK(list.erase(1));
    CHECK(!list.erase(1));
    check_order(list, {2});
    CHECK(list.erase(2));
    CHECK(list.empty());
    CHECK(list.first() == nullptr);
}

TEST(1, "no keys leak") {
    const auto before = Tracked::live();
    {
        IndexedSkipList<TrackedKey> list;
        for (int i = 0; i < 800; ++i) {
            list.insert(TrackedKey(i));
        }
        for (int i = 0; i < 800; i += 2) {
            list.erase(TrackedKey(i));
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "destroying a very long list does not overflow the stack") {
    List list;
    for (int key = 0; key < 200'000; ++key) {
        list.insert(key);
    }
    CHECK_EQ(list.size(), 200'000);
}

TEST(1, "random updates match std::set") {
    Rng rng;
    List list(17);
    std::set<int> model;
    for (int step = 0; step < 15'000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 500));
        if (rng.coin(0.55)) {
            REQUIRE_EQ(list.insert(key) != nullptr, model.insert(key).second);
        } else {
            REQUIRE_EQ(list.erase(key), model.erase(key) == 1);
        }
        const int probe = static_cast<int>(rng.uniform(0, 500));
        REQUIRE_EQ(list.find(probe) != nullptr, model.contains(probe));
    }
    check_order(list, std::vector<int>(model.begin(), model.end()));
}

TEST(1, "a finger already on the key returns immediately") {
    List list;
    auto* node = list.insert(7);
    list.insert(9);
    CHECK(list.finger_search(node, 7) == node);
}

TEST(1, "finger search moves in both directions") {
    List list;
    const auto nodes = filled(list, 1000);
    CHECK(list.finger_search(nodes[100], 101) == nodes[101]);
    CHECK(list.finger_search(nodes[100], 99) == nodes[99]);
    CHECK(list.finger_search(nodes[100], 900) == nodes[900]);
    CHECK(list.finger_search(nodes[900], 100) == nodes[100]);
}

TEST(1, "finger search reaches both ends") {
    List list;
    const auto nodes = filled(list, 500);
    CHECK(list.finger_search(nodes[250], 0) == nodes[0]);
    CHECK(list.finger_search(nodes[250], 499) == nodes[499]);
    CHECK(list.finger_search(nodes[0], 499) == nodes[499]);
    CHECK(list.finger_search(nodes[499], 0) == nodes[0]);
}

TEST(1, "a missing key yields nullptr") {
    List list;
    for (int key = 0; key < 100; key += 2) {
        list.insert(key);
    }
    auto* from = list.find(50);
    CHECK(list.finger_search(from, 51) == nullptr);
    CHECK(list.finger_search(from, 49) == nullptr);
    CHECK(list.finger_search(from, 1000) == nullptr);
    CHECK(list.finger_search(from, -5) == nullptr);
}

TEST(1, "random finger searches agree with a search from the head") {
    Rng rng;
    List list(23);
    std::vector<List::Node*> nodes;
    std::set<int> model;
    for (int i = 0; i < 2000; ++i) {
        const int key = static_cast<int>(rng.uniform(0, 5000));
        if (auto* node = list.insert(key)) {
            nodes.push_back(node);
            model.insert(key);
        }
    }
    for (int step = 0; step < 40'000; ++step) {
        const int probe = static_cast<int>(rng.uniform(0, 5000));
        auto* from = nodes[static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(nodes.size()) - 1))];
        REQUIRE(list.finger_search(from, probe) == list.find(probe));
    }
}

TEST(1, "finger search cost depends on the distance, not the list size") {
    Rng rng;
    const int n = 1 << 16;
    IndexedSkipList<Counted<int>> list(4242);
    std::vector<IndexedSkipList<Counted<int>>::Node*> nodes(static_cast<std::size_t>(n));
    for (int key : rng.permutation(static_cast<std::size_t>(n))) {
        nodes[static_cast<std::size_t>(key)] = list.insert(Counted<int>(key));
    }
    for (int distance : {1, 4, 64, 1024, 16384}) {
        INFO("distance ", distance);
        std::uint64_t total = 0;
        const int queries = 4000;
        for (int query = 0; query < queries; ++query) {
            const bool forward = rng.coin();
            const int start = static_cast<int>(rng.uniform(forward ? 0 : distance, forward ? n - 1 - distance : n - 1));
            const int target = forward ? start + distance : start - distance;
            const Counted<int> key(target);
            Counted<int>::comparisons() = 0;
            REQUIRE(list.finger_search(nodes[static_cast<std::size_t>(start)], key) ==
                    nodes[static_cast<std::size_t>(target)]);
            total += Counted<int>::comparisons();
        }
        CHECK_LE(static_cast<double>(total) / queries, average_cost_limit(distance));
    }
}

TEST(2, "rank is the sorted position") {
    List list;
    for (int key : {30, 10, 20}) {
        list.insert(key);
    }
    CHECK_EQ(list.rank(list.find(10)), 0);
    CHECK_EQ(list.rank(list.find(20)), 1);
    CHECK_EQ(list.rank(list.find(30)), 2);
}

TEST(2, "at returns the node at a position") {
    List list;
    for (int key : {30, 10, 20}) {
        list.insert(key);
    }
    CHECK_EQ(list.at(0)->key(), 10);
    CHECK_EQ(list.at(2)->key(), 30);
    CHECK(list.at(3) == nullptr);
}

TEST(2, "an empty list has no positions") {
    const List list;
    CHECK(list.at(0) == nullptr);
}

TEST(2, "inserting and erasing shift later positions") {
    List list;
    for (int key = 0; key < 100; key += 10) {
        list.insert(key);
    }
    list.insert(5);
    CHECK_EQ(list.rank(list.find(10)), 2);
    list.erase(0);
    CHECK_EQ(list.rank(list.find(10)), 1);
    CHECK_EQ(list.at(0)->key(), 5);
}

TEST(2, "random updates keep every position correct") {
    Rng rng;
    List list(31);
    std::set<int> model;
    for (int step = 0; step < 6000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 400));
        if (rng.coin(0.6)) {
            REQUIRE_EQ(list.insert(key) != nullptr, model.insert(key).second);
        } else {
            REQUIRE_EQ(list.erase(key), model.erase(key) == 1);
        }
        const std::vector<int> sorted(model.begin(), model.end());
        if (!sorted.empty()) {
            const auto index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(sorted.size()) - 1));
            REQUIRE_EQ(list.at(index)->key(), sorted[index]);
            REQUIRE_EQ(list.rank(list.find(sorted[index])), index);
        }
        REQUIRE(list.at(sorted.size()) == nullptr);
    }
}

TEST(2, "every position is correct after a large build") {
    List list;
    const auto nodes = filled(list, 5000);
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        REQUIRE_EQ(list.rank(nodes[i]), i);
        REQUIRE(list.at(i) == nodes[i]);
    }
}

TEST(2, "rank and at walk O(log n) links") {
    Rng rng;
    List list;
    const int n = 1 << 16;
    const auto nodes = filled(list, n);
    const int queries = 5000;
    const std::uint64_t before_rank = list.steps();
    for (int query = 0; query < queries; ++query) {
        list.rank(nodes[static_cast<std::size_t>(rng.uniform(0, n - 1))]);
    }
    CHECK_GT(list.steps(), before_rank);
    CHECK_LE(static_cast<double>(list.steps() - before_rank) / queries, 4.0 * 16);
    const std::uint64_t before_at = list.steps();
    for (int query = 0; query < queries; ++query) {
        list.at(static_cast<std::size_t>(rng.uniform(0, n - 1)));
    }
    CHECK_GT(list.steps(), before_at);
    CHECK_LE(static_cast<double>(list.steps() - before_at) / queries, 4.0 * 16);
}

PERF(2, "positional queries on a large list", 3500) {
    Rng rng;
    List list;
    const int n = 300'000;
    const auto nodes = filled(list, n);
    std::uint64_t sum = 0;
    for (int query = 0; query < 300'000; ++query) {
        sum += list.rank(nodes[static_cast<std::size_t>(rng.uniform(0, n - 1))]);
        sum += static_cast<std::uint64_t>(list.at(static_cast<std::size_t>(rng.uniform(0, n - 1)))->key());
    }
    CHECK_GT(sum, 0);
}

TEST(3, "a zero jump returns the same node") {
    List list;
    auto* node = list.insert(1);
    CHECK(list.rank_search(node, 0) == node);
}

TEST(3, "one step in each direction reaches the neighbors") {
    List list;
    const auto nodes = filled(list, 100);
    CHECK(list.rank_search(nodes[50], 1) == nodes[51]);
    CHECK(list.rank_search(nodes[50], -1) == nodes[49]);
}

TEST(3, "a jump may land exactly on either end") {
    List list;
    const auto nodes = filled(list, 100);
    CHECK(list.rank_search(nodes[50], 49) == nodes[99]);
    CHECK(list.rank_search(nodes[50], -50) == nodes[0]);
    CHECK(list.rank_search(nodes[0], 99) == nodes[99]);
    CHECK(list.rank_search(nodes[99], -99) == nodes[0]);
}

TEST(3, "a jump past either end returns nullptr") {
    List list;
    const auto nodes = filled(list, 100);
    CHECK(list.rank_search(nodes[50], 50) == nullptr);
    CHECK(list.rank_search(nodes[50], -51) == nullptr);
    CHECK(list.rank_search(nodes[99], 1) == nullptr);
    CHECK(list.rank_search(nodes[0], -1) == nullptr);
    CHECK(list.rank_search(nodes[50], 1'000'000) == nullptr);
}

TEST(3, "random jumps agree with positional lookup") {
    Rng rng;
    List list;
    const int n = 3000;
    const auto nodes = filled(list, n);
    for (int step = 0; step < 60'000; ++step) {
        const auto from = rng.uniform(0, n - 1);
        const auto delta = rng.uniform(-n, n);
        const auto target = from + delta;
        INFO("from ", from, " delta ", delta);
        const auto* expected = target >= 0 && target < n ? nodes[static_cast<std::size_t>(target)] : nullptr;
        REQUIRE(list.rank_search(nodes[static_cast<std::size_t>(from)], delta) == expected);
    }
}

TEST(3, "jumps stay correct as the list changes") {
    Rng rng;
    List list(41);
    std::set<int> model;
    for (int step = 0; step < 6000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 300));
        if (rng.coin(0.6)) {
            list.insert(key);
            model.insert(key);
        } else {
            list.erase(key);
            model.erase(key);
        }
        if (model.empty()) {
            continue;
        }
        const std::vector<int> sorted(model.begin(), model.end());
        const auto size = static_cast<std::int64_t>(sorted.size());
        const auto from = rng.uniform(0, size - 1);
        const auto delta = rng.uniform(-size, size);
        const auto target = from + delta;
        auto* result = list.rank_search(list.find(sorted[static_cast<std::size_t>(from)]), delta);
        if (target < 0 || target >= size) {
            REQUIRE(result == nullptr);
        } else {
            REQUIRE(result != nullptr);
            REQUIRE_EQ(result->key(), sorted[static_cast<std::size_t>(target)]);
        }
    }
}

TEST(3, "jump cost depends on the distance, not the list size") {
    Rng rng;
    List list;
    const int n = 1 << 16;
    const auto nodes = filled(list, n);
    for (int distance : {1, 4, 64, 1024, 16384}) {
        INFO("distance ", distance);
        const int queries = 4000;
        const std::uint64_t before = list.steps();
        for (int query = 0; query < queries; ++query) {
            const bool forward = rng.coin();
            const int start = static_cast<int>(rng.uniform(forward ? 0 : distance, forward ? n - 1 - distance : n - 1));
            const int target = forward ? start + distance : start - distance;
            REQUIRE(list.rank_search(nodes[static_cast<std::size_t>(start)], forward ? distance : -distance) ==
                    nodes[static_cast<std::size_t>(target)]);
        }
        CHECK_GT(list.steps(), before);
        CHECK_LE(static_cast<double>(list.steps() - before) / queries, average_cost_limit(distance));
    }
}

PERF(3, "long jumps on a large list", 3500) {
    Rng rng;
    List list;
    const int n = 300'000;
    const auto nodes = filled(list, n);
    std::uint64_t sum = 0;
    for (int query = 0; query < 300'000; ++query) {
        const std::int64_t start = rng.uniform(0, n / 2 - 1);
        const std::int64_t jump = rng.uniform(n / 4, n / 2 - 1);
        sum += static_cast<std::uint64_t>(list.rank_search(nodes[static_cast<std::size_t>(start)], jump)->key());
    }
    CHECK_GT(sum, 0);
}

TEST(4, "an empty window has no median") {
    const SlidingMedian window;
    CHECK(!window.median().has_value());
    CHECK_EQ(window.size(), 0);
}

TEST(4, "removing from an empty window throws") {
    SlidingMedian window;
    CHECK_THROWS_AS(window.pop(), std::out_of_range);
}

TEST(4, "a single value is its own median") {
    SlidingMedian window;
    window.push(7);
    CHECK_EQ(window.median(), std::optional<std::int64_t>(7));
    window.pop();
    CHECK(!window.median().has_value());
}

TEST(4, "an even window reports the lower median") {
    SlidingMedian window;
    window.push(10);
    window.push(20);
    CHECK_EQ(window.median(), std::optional<std::int64_t>(10));
    window.push(30);
    CHECK_EQ(window.median(), std::optional<std::int64_t>(20));
    window.push(5);
    CHECK_EQ(window.median(), std::optional<std::int64_t>(10));
}

TEST(4, "pop removes the oldest value") {
    SlidingMedian window;
    for (std::int64_t value : {1, 100, 50}) {
        window.push(value);
    }
    window.pop();
    CHECK_EQ(window.size(), 2);
    CHECK_EQ(window.median(), std::optional<std::int64_t>(50));
    window.pop();
    CHECK_EQ(window.median(), std::optional<std::int64_t>(50));
}

TEST(4, "duplicates are counted individually") {
    SlidingMedian window;
    for (std::int64_t value : {5, 5, 5, 1, 9}) {
        window.push(value);
    }
    CHECK_EQ(window.median(), std::optional<std::int64_t>(5));
    window.pop();
    window.pop();
    window.pop();
    CHECK_EQ(window.median(), std::optional<std::int64_t>(1));
}

TEST(4, "random pushes and pops match a sorted window") {
    Rng rng;
    SlidingMedian window(11);
    std::vector<std::int64_t> arrivals;
    std::size_t oldest = 0;
    std::multiset<std::int64_t> model;
    for (int step = 0; step < 30'000; ++step) {
        INFO("step ", step);
        if (model.empty() || rng.coin(0.52)) {
            const std::int64_t value = rng.uniform(-50, 50);
            window.push(value);
            arrivals.push_back(value);
            model.insert(value);
        } else {
            window.pop();
            model.erase(model.find(arrivals[oldest++]));
        }
        REQUIRE_EQ(window.size(), model.size());
        if (model.empty()) {
            REQUIRE(!window.median().has_value());
        } else {
            const auto middle = std::next(model.begin(), static_cast<std::ptrdiff_t>((model.size() - 1) / 2));
            REQUIRE_EQ(*window.median(), *middle);
        }
    }
}

TEST(4, "the median moves by nearby jumps, not searches from the head") {
    Rng rng;
    SlidingMedian window(77);
    const int span = 1 << 15;
    const int operations = 60'000;
    for (int i = 0; i < span; ++i) {
        window.push(rng.uniform(0, 1'000'000'000));
    }
    const std::uint64_t before = window.steps();
    for (int i = 0; i < operations; ++i) {
        window.push(rng.uniform(0, 1'000'000'000));
        window.pop();
    }
    CHECK_LE(static_cast<double>(window.steps() - before) / operations, 16.0);
}

PERF(4, "a long stream with a wide window", 3500) {
    Rng rng;
    SlidingMedian window(5);
    std::int64_t sum = 0;
    for (int i = 0; i < 1'200'000; ++i) {
        window.push(rng.uniform(0, 1'000'000'000));
        if (window.size() > 50'000) {
            window.pop();
        }
        sum += *window.median();
    }
    CHECK_GT(sum, 0);
}
