#include <list>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;
using List = DoublyLinkedList<int>;

namespace {

template <typename T>
std::vector<int> values_of(const DoublyLinkedList<T>& list) {
    std::vector<int> out;
    for (auto* node = list.head(); node != nullptr; node = node->next) {
        if constexpr (std::is_same_v<T, Tracked>) {
            out.push_back(node->item.value());
        } else {
            out.push_back(node->item);
        }
    }
    return out;
}

// Walks both directions and confirms the prev/next pointers and the end pointers agree.
template <typename T>
void check_links(const DoublyLinkedList<T>& list) {
    if (list.head() == nullptr || list.tail() == nullptr) {
        CHECK(list.head() == nullptr);
        CHECK(list.tail() == nullptr);
        return;
    }
    CHECK(list.head()->prev == nullptr);
    CHECK(list.tail()->next == nullptr);
    auto* node = list.head();
    while (node->next != nullptr) {
        REQUIRE(node->next->prev == node);
        node = node->next;
    }
    CHECK(node == list.tail());
}

std::vector<List::Node*> build(List& list, const std::vector<int>& values) {
    std::vector<List::Node*> nodes;
    for (int value : values) {
        nodes.push_back(list.insert_last(value));
    }
    return nodes;
}

}  // namespace

TEST(1, "a new list is empty") {
    List list;
    CHECK(list.empty());
    CHECK(list.head() == nullptr);
    CHECK(list.tail() == nullptr);
}

TEST(1, "insert at both ends keeps order") {
    List list;
    list.insert_last(2);
    list.insert_first(1);
    list.insert_last(3);
    CHECK_EQ(values_of(list), std::vector<int>{1, 2, 3});
    check_links(list);
}

TEST(1, "insert returns the node that holds the value") {
    List list;
    auto* a = list.insert_first(10);
    auto* b = list.insert_last(20);
    CHECK_EQ(a->item, 10);
    CHECK_EQ(b->item, 20);
    CHECK(a == list.head());
    CHECK(b == list.tail());
}

TEST(1, "delete returns values from each end") {
    List list;
    build(list, {1, 2, 3, 4});
    CHECK_EQ(list.delete_first(), 1);
    CHECK_EQ(list.delete_last(), 4);
    CHECK_EQ(values_of(list), std::vector<int>{2, 3});
    check_links(list);
}

TEST(1, "deleting the only node empties both ends") {
    List from_front;
    from_front.insert_last(7);
    CHECK_EQ(from_front.delete_first(), 7);
    CHECK(from_front.empty());
    check_links(from_front);

    List from_back;
    from_back.insert_first(8);
    CHECK_EQ(from_back.delete_last(), 8);
    CHECK(from_back.empty());
    check_links(from_back);
}

TEST(1, "deleting from an empty list throws out_of_range") {
    List list;
    CHECK_THROWS_AS(list.delete_first(), std::out_of_range);
    CHECK_THROWS_AS(list.delete_last(), std::out_of_range);
    list.insert_last(1);
    list.delete_last();
    CHECK_THROWS_AS(list.delete_last(), std::out_of_range);
}

TEST(1, "destructor releases every element") {
    const auto before = Tracked::live();
    {
        DoublyLinkedList<Tracked> list;
        for (int i = 0; i < 100; ++i) {
            list.insert_last(Tracked(i));
        }
        CHECK_EQ(list.delete_first().value(), 0);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "destructing a very long list does not overflow the stack") {
    List list;
    for (int i = 0; i < 1'000'000; ++i) {
        list.insert_first(i);
    }
}

TEST(1, "random operations match std::list") {
    Rng rng;
    List list;
    std::list<int> model;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const auto op = rng.uniform(0, 3);
        const int value = static_cast<int>(rng.uniform(0, 1'000'000));
        if (op == 0) {
            list.insert_first(value);
            model.push_front(value);
        } else if (op == 1) {
            list.insert_last(value);
            model.push_back(value);
        } else if (!model.empty() && op == 2) {
            REQUIRE_EQ(list.delete_first(), model.front());
            model.pop_front();
        } else if (!model.empty()) {
            REQUIRE_EQ(list.delete_last(), model.back());
            model.pop_back();
        }
    }
    CHECK_EQ(values_of(list), std::vector<int>(model.begin(), model.end()));
    check_links(list);
}

TEST(2, "remove from the middle") {
    List list;
    auto nodes = build(list, {1, 2, 3, 4, 5});
    List cut = list.remove(nodes[1], nodes[3]);
    CHECK_EQ(values_of(list), std::vector<int>{1, 5});
    CHECK_EQ(values_of(cut), std::vector<int>{2, 3, 4});
    check_links(list);
    check_links(cut);
}

TEST(2, "remove moves the nodes rather than copying them") {
    List list;
    auto nodes = build(list, {1, 2, 3});
    List cut = list.remove(nodes[1], nodes[1]);
    CHECK(cut.head() == nodes[1]);
    CHECK(cut.tail() == nodes[1]);
}

TEST(2, "remove a prefix") {
    List list;
    auto nodes = build(list, {1, 2, 3, 4});
    List cut = list.remove(nodes[0], nodes[1]);
    CHECK_EQ(values_of(list), std::vector<int>{3, 4});
    CHECK_EQ(values_of(cut), std::vector<int>{1, 2});
    CHECK(list.head() == nodes[2]);
    check_links(list);
    check_links(cut);
}

TEST(2, "remove a suffix") {
    List list;
    auto nodes = build(list, {1, 2, 3, 4});
    List cut = list.remove(nodes[2], nodes[3]);
    CHECK_EQ(values_of(list), std::vector<int>{1, 2});
    CHECK_EQ(values_of(cut), std::vector<int>{3, 4});
    CHECK(list.tail() == nodes[1]);
    check_links(list);
    check_links(cut);
}

TEST(2, "remove the whole list") {
    List list;
    auto nodes = build(list, {1, 2, 3});
    List cut = list.remove(nodes[0], nodes[2]);
    CHECK(list.empty());
    CHECK_EQ(values_of(cut), std::vector<int>{1, 2, 3});
    check_links(list);
    check_links(cut);
}

TEST(2, "the source list keeps working after a remove") {
    List list;
    auto nodes = build(list, {1, 2, 3});
    List cut = list.remove(nodes[1], nodes[1]);
    list.insert_last(9);
    cut.insert_first(0);
    CHECK_EQ(values_of(list), std::vector<int>{1, 3, 9});
    CHECK_EQ(values_of(cut), std::vector<int>{0, 2});
    check_links(list);
    check_links(cut);
}

TEST(3, "splice into the middle") {
    List list;
    auto nodes = build(list, {1, 2, 3});
    List other;
    build(other, {7, 8});
    list.splice(nodes[1], other);
    CHECK_EQ(values_of(list), std::vector<int>{1, 2, 7, 8, 3});
    CHECK(other.empty());
    check_links(list);
    check_links(other);
}

TEST(3, "splice after the tail moves the tail pointer") {
    List list;
    auto nodes = build(list, {1, 2});
    List other;
    auto incoming = build(other, {3, 4});
    list.splice(nodes[1], other);
    CHECK_EQ(values_of(list), std::vector<int>{1, 2, 3, 4});
    CHECK(list.tail() == incoming[1]);
    check_links(list);
}

TEST(3, "splicing an empty list changes nothing") {
    List list;
    auto nodes = build(list, {1, 2});
    List other;
    list.splice(nodes[0], other);
    CHECK_EQ(values_of(list), std::vector<int>{1, 2});
    check_links(list);
    CHECK(other.empty());
}

TEST(3, "remove then splice round-trips") {
    List list;
    auto nodes = build(list, {1, 2, 3, 4, 5, 6});
    List cut = list.remove(nodes[1], nodes[2]);
    list.splice(nodes[4], cut);
    CHECK_EQ(values_of(list), std::vector<int>{1, 4, 5, 2, 3, 6});
    CHECK(cut.empty());
    check_links(list);
}

TEST(3, "move construction and assignment transfer ownership") {
    const auto before = Tracked::live();
    {
        DoublyLinkedList<Tracked> a;
        a.insert_last(Tracked(1));
        a.insert_last(Tracked(2));

        DoublyLinkedList<Tracked> b(std::move(a));
        CHECK(a.empty());  // NOLINT: moved-from state is part of this class's contract
        CHECK_EQ(values_of(b), std::vector<int>{1, 2});

        DoublyLinkedList<Tracked> c;
        c.insert_last(Tracked(9));
        c = std::move(b);
        CHECK(b.empty());  // NOLINT
        CHECK_EQ(values_of(c), std::vector<int>{1, 2});
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(3, "random cuts and splices match a model") {
    Rng rng;
    List list;
    std::vector<List::Node*> order;
    std::vector<int> model;
    for (int i = 0; i < 400; ++i) {
        order.push_back(list.insert_last(i));
        model.push_back(i);
    }
    for (int step = 0; step < 3'000; ++step) {
        INFO("step ", step);
        const auto n = static_cast<std::int64_t>(order.size());
        const auto left = rng.uniform(0, n - 1);
        const auto right = rng.uniform(left, n - 1);
        const auto l = static_cast<std::size_t>(left);
        const auto r = static_cast<std::size_t>(right);

        std::vector<List::Node*> cut_nodes(order.begin() + left, order.begin() + right + 1);
        std::vector<int> cut_values(model.begin() + left, model.begin() + right + 1);
        List cut = list.remove(order[l], order[r]);
        order.erase(order.begin() + left, order.begin() + right + 1);
        model.erase(model.begin() + left, model.begin() + right + 1);

        if (order.empty()) {
            list = std::move(cut);
            order = cut_nodes;
            model = cut_values;
            continue;
        }
        const auto anchor = rng.uniform(0, static_cast<std::int64_t>(order.size()) - 1);
        list.splice(order[static_cast<std::size_t>(anchor)], cut);
        order.insert(order.begin() + anchor + 1, cut_nodes.begin(), cut_nodes.end());
        model.insert(model.begin() + anchor + 1, cut_values.begin(), cut_values.end());
    }
    CHECK_EQ(values_of(list), model);
    check_links(list);
}

PERF(1, "two million pushes and pops", 1500) {
    List list;
    for (int i = 0; i < 1'000'000; ++i) {
        list.insert_first(i);
        list.insert_last(i);
    }
    long long sum = 0;
    while (!list.empty()) {
        sum += list.delete_first();
    }
    CHECK_GT(sum, 0);
}

// A walk over the long range in remove or splice costs billions of steps here.
PERF(3, "remove and splice do not walk the range", 300) {
    List list;
    std::vector<List::Node*> nodes;
    for (int i = 0; i < 200'000; ++i) {
        nodes.push_back(list.insert_last(i));
    }
    for (int round = 0; round < 200'000; ++round) {
        List cut = list.remove(nodes[1], nodes[199'998]);
        list.splice(nodes[0], cut);
    }
    CHECK_EQ(list.head()->next, nodes[1]);
    CHECK_EQ(list.tail()->prev, nodes[199'998]);
}

TEST(4, "get returns what put stored") {
    LruCache<std::string, int> cache(2);
    CHECK(!cache.put("a", 1).has_value());
    CHECK_EQ(cache.get("a"), std::optional<int>(1));
    CHECK_EQ(cache.get("missing"), std::optional<int>());
    CHECK_EQ(cache.size(), 1);
}

TEST(4, "the least recently used key is evicted and reported") {
    LruCache<std::string, int> cache(2);
    cache.put("a", 1);
    cache.put("b", 2);
    CHECK_EQ(cache.put("c", 3), std::optional<std::string>("a"));
    CHECK_EQ(cache.get("a"), std::optional<int>());
    CHECK_EQ(cache.size(), 2);
}

TEST(4, "get refreshes recency") {
    LruCache<std::string, int> cache(2);
    cache.put("a", 1);
    cache.put("b", 2);
    cache.get("a");
    CHECK_EQ(cache.put("c", 3), std::optional<std::string>("b"));
    CHECK_EQ(cache.get("a"), std::optional<int>(1));
}

TEST(4, "overwriting updates the value, refreshes recency and evicts nothing") {
    LruCache<std::string, int> cache(2);
    cache.put("a", 1);
    cache.put("b", 2);
    CHECK(!cache.put("a", 10).has_value());
    CHECK_EQ(cache.size(), 2);
    CHECK_EQ(cache.get("a"), std::optional<int>(10));
    CHECK_EQ(cache.put("c", 3), std::optional<std::string>("b"));
}

TEST(4, "capacity one always keeps the newest key") {
    LruCache<int, int> cache(1);
    cache.put(1, 1);
    CHECK_EQ(cache.put(2, 2), std::optional<int>(1));
    CHECK_EQ(cache.get(1), std::optional<int>());
    CHECK_EQ(cache.get(2), std::optional<int>(2));
}

TEST(4, "random workload matches a brute-force model") {
    Rng rng;
    const std::size_t capacity = 16;
    LruCache<int, int> cache(capacity);
    std::list<std::pair<int, int>> model;  // most recent first

    const auto find = [&](int key) {
        return std::find_if(model.begin(), model.end(),
                            [&](const auto& entry) { return entry.first == key; });
    };
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 40));
        if (rng.coin()) {
            const auto it = find(key);
            std::optional<int> expected;
            if (it != model.end()) {
                expected = it->second;
                model.splice(model.begin(), model, it);
            }
            REQUIRE_EQ(cache.get(key), expected);
        } else {
            const int value = static_cast<int>(rng.uniform(0, 1000));
            std::optional<int> evicted;
            const auto it = find(key);
            if (it != model.end()) {
                it->second = value;
                model.splice(model.begin(), model, it);
            } else {
                if (model.size() == capacity) {
                    evicted = model.back().first;
                    model.pop_back();
                }
                model.emplace_front(key, value);
            }
            REQUIRE_EQ(cache.put(key, value), evicted);
        }
        REQUIRE_EQ(cache.size(), model.size());
    }
}

PERF(4, "operations stay O(1) when the cache is huge", 2000) {
    LruCache<int, int> cache(500'000);
    for (int i = 0; i < 1'000'000; ++i) {
        cache.put(i, i);
    }
    long long hits = 0;
    for (int i = 0; i < 1'000'000; ++i) {
        hits += cache.get(i).has_value() ? 1 : 0;
    }
    CHECK_EQ(hits, 500'000);
}
