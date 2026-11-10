#include <algorithm>
#include <cstddef>
#include <forward_list>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "00_alloc_counter.hpp"
#include "00_counters.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;
using practice6006::AllocationScope;
using practice6006::Probe;
using List = LinkedListSeq<int>;

namespace {

std::vector<int> values_of(const List& list) {
    std::vector<int> out;
    for (auto* node = list.head(); node != nullptr; node = node->next) {
        out.push_back(node->item);
    }
    return out;
}

List make_list(const std::vector<int>& values) { return List(values); }

std::vector<List::Node*> nodes_of(const List& list) {
    std::vector<List::Node*> out;
    for (auto* node = list.head(); node != nullptr; node = node->next) {
        out.push_back(node);
    }
    return out;
}

struct RhoNode {
    int id = 0;
    RhoNode* next = nullptr;
};

// `tail` nodes followed by a cycle of `cycle` nodes (no cycle when cycle == 0).
std::vector<RhoNode> build_rho(std::size_t tail, std::size_t cycle) {
    std::vector<RhoNode> nodes(tail + cycle);
    for (std::size_t i = 0; i + 1 < nodes.size(); ++i) {
        nodes[i].next = &nodes[i + 1];
    }
    if (cycle > 0) {
        nodes.back().next = &nodes[tail];
    }
    return nodes;
}

}  // namespace

TEST(1, "a new list is empty") {
    List list;
    CHECK(list.empty());
    CHECK_EQ(list.size(), 0);
    CHECK(list.head() == nullptr);
}

TEST(1, "insert_first pushes to the front and returns the node") {
    List list;
    auto* a = list.insert_first(1);
    auto* b = list.insert_first(2);
    CHECK_EQ(values_of(list), std::vector<int>{2, 1});
    CHECK_EQ(list.size(), 2);
    CHECK_EQ(a->item, 1);
    CHECK(list.head() == b);
}

TEST(1, "delete_first pops from the front") {
    auto list = make_list({1, 2, 3});
    CHECK_EQ(list.delete_first(), 1);
    CHECK_EQ(list.delete_first(), 2);
    CHECK_EQ(list.size(), 1);
    CHECK_EQ(values_of(list), std::vector<int>{3});
}

TEST(1, "deleting the only node empties the list") {
    List list;
    list.insert_first(7);
    CHECK_EQ(list.delete_first(), 7);
    CHECK(list.empty());
    CHECK(list.head() == nullptr);
}

TEST(1, "deleting from an empty list throws out_of_range") {
    List list;
    CHECK_THROWS_AS(list.delete_first(), std::out_of_range);
}

TEST(1, "construction from a span keeps order") {
    const std::vector<int> values = {4, 5, 6};
    List list(values);
    CHECK_EQ(values_of(list), values);
    CHECK_EQ(list.size(), 3);
    const std::vector<int> none;
    List empty(none);
    CHECK(empty.empty());
}

TEST(1, "the destructor releases every element") {
    const auto before = Tracked::live();
    {
        LinkedListSeq<Tracked> list;
        for (int i = 0; i < 100; ++i) {
            list.insert_first(Tracked(i));
        }
        CHECK_EQ(list.delete_first().value(), 99);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "destroying a very long list does not overflow the stack") {
    List list;
    for (int i = 0; i < 1'000'000; ++i) {
        list.insert_first(i);
    }
}

TEST(1, "move construction and assignment transfer ownership") {
    const auto before = Tracked::live();
    {
        LinkedListSeq<Tracked> a;
        a.insert_first(Tracked(1));
        a.insert_first(Tracked(2));

        LinkedListSeq<Tracked> b(std::move(a));
        CHECK(a.empty());  // NOLINT: moved-from state is part of this class's contract
        CHECK_EQ(b.size(), 2);

        LinkedListSeq<Tracked> c;
        c.insert_first(Tracked(9));
        c = std::move(b);
        CHECK(b.empty());  // NOLINT
        CHECK_EQ(c.size(), 2);
        CHECK_EQ(c.head()->item.value(), 2);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "moves are noexcept") {
    List list;  // reports "not implemented" until the class exists
    CHECK(std::is_nothrow_move_constructible_v<List>);
    CHECK(std::is_nothrow_move_assignable_v<List>);
}

TEST(1, "random pushes and pops match std::forward_list") {
    Rng rng;
    List list;
    std::forward_list<int> model;
    std::size_t model_size = 0;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        if (rng.coin(0.55) || model.empty()) {
            const int value = static_cast<int>(rng.uniform(0, 1'000'000));
            list.insert_first(value);
            model.push_front(value);
            ++model_size;
        } else {
            REQUIRE_EQ(list.delete_first(), model.front());
            model.pop_front();
            --model_size;
        }
        REQUIRE_EQ(list.size(), model_size);
    }
    CHECK_EQ(values_of(list), std::vector<int>(model.begin(), model.end()));
}

TEST(2, "get_at and set_at reach the right position") {
    auto list = make_list({10, 20, 30});
    CHECK_EQ(list.get_at(0), 10);
    CHECK_EQ(list.get_at(2), 30);
    list.set_at(1, 21);
    CHECK_EQ(list.get_at(1), 21);
    CHECK_EQ(values_of(list), std::vector<int>{10, 21, 30});
}

TEST(2, "out-of-range indexes throw out_of_range and change nothing") {
    auto list = make_list({1, 2});
    CHECK_THROWS_AS(list.get_at(2), std::out_of_range);
    CHECK_THROWS_AS(list.set_at(2, 5), std::out_of_range);
    CHECK_THROWS_AS(list.insert_at(3, 5), std::out_of_range);
    CHECK_THROWS_AS(list.delete_at(2), std::out_of_range);
    CHECK_EQ(values_of(list), std::vector<int>{1, 2});
    List empty;
    CHECK_THROWS_AS(empty.delete_last(), std::out_of_range);
    CHECK_THROWS_AS(empty.get_at(0), std::out_of_range);
}

TEST(2, "insert_at front, middle and end") {
    auto list = make_list({10, 20, 30});
    auto* front = list.insert_at(0, 5);
    CHECK_EQ(front->item, 5);
    list.insert_at(2, 15);
    auto* back = list.insert_at(list.size(), 35);
    CHECK_EQ(back->item, 35);
    CHECK_EQ(values_of(list), std::vector<int>{5, 10, 15, 20, 30, 35});
    CHECK_EQ(list.size(), 6);
}

TEST(2, "delete_at front, middle and last") {
    auto list = make_list({1, 2, 3, 4, 5});
    CHECK_EQ(list.delete_at(0), 1);
    CHECK_EQ(list.delete_at(1), 3);
    CHECK_EQ(list.delete_at(2), 5);
    CHECK_EQ(values_of(list), std::vector<int>{2, 4});
    CHECK_EQ(list.size(), 2);
}

TEST(2, "insert_last and delete_last") {
    List list;
    list.insert_last(1);
    list.insert_last(2);
    CHECK_EQ(values_of(list), std::vector<int>{1, 2});
    CHECK_EQ(list.delete_last(), 2);
    CHECK_EQ(list.delete_last(), 1);
    CHECK(list.empty());
}

TEST(2, "indexed operations release memory correctly") {
    const auto before = Tracked::live();
    {
        LinkedListSeq<Tracked> list;
        for (int i = 0; i < 50; ++i) {
            list.insert_at(list.size() / 2, Tracked(i));
        }
        for (int i = 0; i < 25; ++i) {
            list.delete_at(list.size() / 3);
        }
        list.delete_last();
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(2, "iterators walk the list in order and can write") {
    auto list = make_list({1, 2, 3});
    for (int& value : list) {
        value *= 10;
    }
    const List& view = list;
    std::vector<int> seen;
    for (const int value : view) {
        seen.push_back(value);
    }
    CHECK_EQ(seen, std::vector<int>{10, 20, 30});
    CHECK(list.begin() != list.end());
    CHECK(++list.begin() != list.end());
}

TEST(2, "an empty list has begin equal to end") {
    List list;
    CHECK(list.begin() == list.end());
}

TEST(2, "iterators work with standard algorithms") {
    auto list = make_list({4, 8, 15, 16});
    const auto found = std::find(list.begin(), list.end(), 15);
    REQUIRE(found != list.end());
    CHECK_EQ(*found, 15);
    CHECK_EQ(std::distance(list.begin(), list.end()), 4);
    auto it = list.begin();
    const auto old = it++;
    CHECK_EQ(*old, 4);
    CHECK_EQ(*it, 8);
    CHECK(std::forward_iterator<List::iterator>);
    CHECK(std::forward_iterator<List::const_iterator>);
    CHECK(old != it);
    CHECK(it == it);
}

TEST(2, "operator-> reaches the element") {
    LinkedListSeq<std::string> list;
    list.insert_first("hello");
    CHECK_EQ(list.begin()->size(), 5);
}

TEST(2, "random operations match std::vector") {
    Rng rng;
    List list;
    std::vector<int> model;
    for (int step = 0; step < 6'000; ++step) {
        INFO("step ", step);
        const auto op = rng.uniform(0, 4);
        const int value = static_cast<int>(rng.uniform(0, 1'000'000));
        if (op == 0 || model.empty()) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size())));
            list.insert_at(i, value);
            model.insert(model.begin() + static_cast<std::ptrdiff_t>(i), value);
        } else if (op == 1) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            REQUIRE_EQ(list.delete_at(i), model[i]);
            model.erase(model.begin() + static_cast<std::ptrdiff_t>(i));
        } else if (op == 2) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            REQUIRE_EQ(list.get_at(i), model[i]);
        } else if (op == 3) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            list.set_at(i, value);
            model[i] = value;
        } else if (rng.coin()) {
            list.insert_last(value);
            model.push_back(value);
        } else {
            REQUIRE_EQ(list.delete_last(), model.back());
            model.pop_back();
        }
        REQUIRE_EQ(list.size(), model.size());
    }
    CHECK_EQ(values_of(list), model);
}

// An O(n) get_at would spend 2 * 10^12 steps here.
PERF(2, "get_at near the front does not depend on the list length", 300) {
    List list;
    for (int i = 0; i < 1'000'000; ++i) {
        list.insert_first(i);
    }
    long long sum = 0;
    for (int i = 0; i < 2'000'000; ++i) {
        sum += list.get_at(static_cast<std::size_t>(i % 8));
    }
    CHECK_GT(sum, 0);
}

TEST(3, "reversing an empty list and a singleton") {
    List empty;
    empty.reverse();
    CHECK(empty.empty());
    auto one = make_list({7});
    one.reverse();
    CHECK_EQ(values_of(one), std::vector<int>{7});
}

TEST(3, "reversing keeps the same nodes") {
    auto list = make_list({1, 2, 3, 4, 5});
    auto before = nodes_of(list);
    list.reverse();
    std::reverse(before.begin(), before.end());
    CHECK_EQ(nodes_of(list), before);
    CHECK_EQ(values_of(list), std::vector<int>{5, 4, 3, 2, 1});
    CHECK_EQ(list.size(), 5);
}

TEST(3, "the list remains usable after reversing") {
    auto list = make_list({1, 2, 3});
    list.reverse();
    list.insert_first(9);
    list.insert_last(0);
    CHECK_EQ(list.delete_first(), 9);
    CHECK_EQ(values_of(list), std::vector<int>{3, 2, 1, 0});
    list.reverse();
    CHECK_EQ(values_of(list), std::vector<int>{0, 1, 2, 3});
}

TEST(3, "reversing moves and copies no elements and allocates nothing") {
    LinkedListSeq<Probe<int>> list;
    for (int i = 0; i < 1'000; ++i) {
        list.insert_first(Probe<int>(i));
    }
    Probe<int>::reset();
    const AllocationScope scope;
    list.reverse();
    CHECK_EQ(Probe<int>::writes(), 0);
    CHECK_EQ(scope.allocations(), 0);
}

TEST(3, "reversing a very long list does not overflow the stack") {
    List list;
    for (int i = 0; i < 1'000'000; ++i) {
        list.insert_first(i);
    }
    list.reverse();
    CHECK_EQ(list.head()->item, 0);
}

TEST(3, "double reversal is the identity on random lists") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 30)), -9, 9);
        auto list = make_list(values);
        list.reverse();
        auto reversed = values;
        std::reverse(reversed.begin(), reversed.end());
        INFO("round ", round);
        REQUIRE_EQ(values_of(list), reversed);
        list.reverse();
        REQUIRE_EQ(values_of(list), values);
    }
}

TEST(4, "a null head has no cycle") {
    CHECK(!find_cycle<RhoNode>(nullptr).has_value());
}

TEST(4, "a terminated list has no cycle") {
    auto nodes = build_rho(5, 0);
    CHECK(!find_cycle<RhoNode>(&nodes[0]).has_value());
}

TEST(4, "a single node pointing to itself") {
    RhoNode node;
    node.next = &node;
    const auto info = find_cycle<RhoNode>(&node);
    REQUIRE(info.has_value());
    CHECK_EQ(info->length, 1);
    CHECK(info->entry == &node);
}

TEST(4, "a single node pointing to nothing") {
    RhoNode node;
    CHECK(!find_cycle<RhoNode>(&node).has_value());
}

TEST(4, "the head can be inside the cycle") {
    auto nodes = build_rho(0, 7);
    const auto info = find_cycle<RhoNode>(&nodes[0]);
    REQUIRE(info.has_value());
    CHECK_EQ(info->length, 7);
    CHECK(info->entry == &nodes[0]);
}

TEST(4, "length and entry for every small rho shape") {
    for (std::size_t tail = 0; tail <= 40; ++tail) {
        for (std::size_t cycle = 1; cycle <= 40; ++cycle) {
            auto nodes = build_rho(tail, cycle);
            INFO("tail=", tail, " cycle=", cycle);
            const auto info = find_cycle<RhoNode>(&nodes[0]);
            REQUIRE(info.has_value());
            CHECK_EQ(info->length, cycle);
            CHECK(info->entry == &nodes[tail]);
        }
    }
}

TEST(4, "no extra heap allocations") {
    auto nodes = build_rho(300, 700);
    const AllocationScope scope;
    const auto info = find_cycle<RhoNode>(&nodes[0]);
    CHECK_EQ(scope.allocations(), 0);
    CHECK(info.has_value());
}

// A hash set of visited nodes is rejected by the allocation test; this catches quadratic walks.
PERF(4, "a rho with millions of nodes", 1000) {
    auto nodes = build_rho(3'000'000, 5'000'000);
    const auto info = find_cycle<RhoNode>(&nodes[0]);
    REQUIRE(info.has_value());
    CHECK_EQ(info->length, 5'000'000);
    CHECK(info->entry == &nodes[3'000'000]);
}
