#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "00_counters.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;
using practice6006::Probe;

namespace {

// Element whose copies can be made to throw, to test exception safety.
class Bomb {
public:
    Bomb() = default;
    explicit Bomb(int value) : tracked_(value) {}
    Bomb(const Bomb& other) : tracked_(other.tracked_) { tick(); }
    Bomb& operator=(const Bomb& other) {
        tick();
        tracked_ = other.tracked_;
        return *this;
    }
    int value() const { return tracked_.value(); }
    friend bool operator==(const Bomb& a, const Bomb& b) { return a.value() == b.value(); }
    friend std::ostream& operator<<(std::ostream& os, const Bomb& b) { return os << b.value(); }

    static void arm(int copies_until_throw) { countdown() = copies_until_throw; }
    static void disarm() { countdown() = -1; }

private:
    static void tick() {
        int& left = countdown();
        if (left == 0) {
            left = -1;
            throw std::runtime_error("copy failed");
        }
        if (left > 0) {
            --left;
        }
    }
    static int& countdown() {
        static int value = -1;
        return value;
    }
    Tracked tracked_;
};

template <typename Array>
std::vector<int> values_of(const Array& array) {
    std::vector<int> out;
    for (const auto& item : array) {
        out.push_back(item.value());
    }
    return out;
}

ArraySequence<int> make_sequence(const std::vector<int>& values) {
    return ArraySequence<int>(values);
}

std::vector<int> contents(const ArraySequence<int>& sequence) {
    return std::vector<int>(sequence.begin(), sequence.end());
}

}  // namespace

TEST(1, "a new array has the requested size and zeroed elements") {
    StaticArray<int> a(4);
    CHECK_EQ(a.size(), 4);
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK_EQ(a.get_at(i), 0);
    }
}

TEST(1, "a zero-length array is valid") {
    StaticArray<int> a(0);
    CHECK_EQ(a.size(), 0);
    CHECK(a.begin() == a.end());
    CHECK_THROWS_AS(a.get_at(0), std::out_of_range);
    StaticArray<int> b;
    CHECK_EQ(b.size(), 0);
}

TEST(1, "set_at then get_at round-trips") {
    StaticArray<std::string> a(3);
    a.set_at(0, "first");
    a.set_at(2, "last");
    CHECK_EQ(a.get_at(0), "first");
    CHECK_EQ(a.get_at(1), "");
    CHECK_EQ(a.get_at(2), "last");
}

TEST(1, "out-of-range access throws and changes nothing") {
    StaticArray<int> a(3);
    a.set_at(1, 7);
    CHECK_THROWS_AS(a.get_at(3), std::out_of_range);
    CHECK_THROWS_AS(a.set_at(3, 9), std::out_of_range);
    CHECK_EQ(a.get_at(1), 7);
}

TEST(1, "operator[] and iteration reach every slot") {
    StaticArray<int> a(5);
    for (std::size_t i = 0; i < a.size(); ++i) {
        a[i] = static_cast<int>(i * i);
    }
    const StaticArray<int>& view = a;
    int sum = 0;
    for (const int value : view) {
        sum += value;
    }
    CHECK_EQ(sum, 0 + 1 + 4 + 9 + 16);
    CHECK_EQ(view[3], 9);
}

TEST(1, "copy construction is deep") {
    StaticArray<int> a(3);
    a.set_at(0, 1);
    StaticArray<int> b(a);
    b.set_at(0, 9);
    CHECK_EQ(a.get_at(0), 1);
    CHECK_EQ(b.get_at(0), 9);
    CHECK_EQ(b.size(), 3);
}

TEST(1, "copy assignment replaces contents and handles self-assignment") {
    StaticArray<int> a(2);
    a.set_at(0, 1);
    StaticArray<int> b(5);
    b = a;
    CHECK_EQ(b.size(), 2);
    CHECK_EQ(b.get_at(0), 1);
    a.set_at(0, 42);
    CHECK_EQ(b.get_at(0), 1);

    a = *&a;
    CHECK_EQ(a.size(), 2);
    CHECK_EQ(a.get_at(0), 42);
}

TEST(1, "move construction and assignment steal the buffer without touching elements") {
    StaticArray<Probe<int>> a(100);
    Probe<int>::reset();
    StaticArray<Probe<int>> b(std::move(a));
    CHECK_EQ(Probe<int>::writes(), 0);
    CHECK_EQ(a.size(), 0);  // NOLINT: moved-from state is part of this class's contract
    CHECK_EQ(b.size(), 100);

    StaticArray<Probe<int>> c(3);
    Probe<int>::reset();
    c = std::move(b);
    CHECK_EQ(Probe<int>::writes(), 0);
    CHECK_EQ(b.size(), 0);  // NOLINT
    CHECK_EQ(c.size(), 100);
}

TEST(1, "moves are noexcept") {
    StaticArray<int> array(1);  // reports "not implemented" until the class exists
    CHECK(std::is_nothrow_move_constructible_v<StaticArray<int>>);
    CHECK(std::is_nothrow_move_assignable_v<StaticArray<int>>);
}

TEST(1, "every element is destroyed exactly once") {
    const auto before = Tracked::live();
    {
        StaticArray<Tracked> a(10);
        a.set_at(3, Tracked(7));
        StaticArray<Tracked> b(a);
        StaticArray<Tracked> c(4);
        c = a;
        StaticArray<Tracked> d(std::move(b));
        d = std::move(c);
        CHECK_EQ(d.get_at(3).value(), 7);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "a failed copy assignment leaves the target unchanged") {
    const auto before = Tracked::live();
    {
        StaticArray<Bomb> source(4);
        for (std::size_t i = 0; i < 4; ++i) {
            source.set_at(i, Bomb(static_cast<int>(i)));
        }
        StaticArray<Bomb> target(2);
        target.set_at(0, Bomb(100));
        target.set_at(1, Bomb(200));

        Bomb::arm(2);
        CHECK_THROWS_AS(target = source, std::runtime_error);
        Bomb::disarm();

        CHECK_EQ(values_of(target), std::vector<int>{100, 200});
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(2, "a new sequence is empty") {
    ArraySequence<int> sequence;
    CHECK_EQ(sequence.size(), 0);
    CHECK(sequence.begin() == sequence.end());
}

TEST(2, "construction from a span copies in order") {
    const std::vector<int> values = {3, 1, 2};
    ArraySequence<int> sequence(values);
    CHECK_EQ(contents(sequence), values);
    CHECK_EQ(sequence.get_at(1), 1);
}

TEST(2, "construction from an empty span gives an empty sequence") {
    const std::vector<int> values;
    ArraySequence<int> sequence(values);
    CHECK_EQ(sequence.size(), 0);
}

TEST(2, "set_at replaces in place") {
    auto sequence = make_sequence({1, 2, 3});
    sequence.set_at(1, 9);
    CHECK_EQ(contents(sequence), std::vector<int>{1, 9, 3});
}

TEST(2, "insert_at front, middle and end") {
    auto sequence = make_sequence({10, 20, 30});
    sequence.insert_at(0, 5);
    CHECK_EQ(contents(sequence), std::vector<int>{5, 10, 20, 30});
    sequence.insert_at(2, 15);
    CHECK_EQ(contents(sequence), std::vector<int>{5, 10, 15, 20, 30});
    sequence.insert_at(5, 35);
    CHECK_EQ(contents(sequence), std::vector<int>{5, 10, 15, 20, 30, 35});
    CHECK_EQ(sequence.size(), 6);
}

TEST(2, "delete_at returns the item and closes the gap") {
    auto sequence = make_sequence({10, 20, 30, 40});
    CHECK_EQ(sequence.delete_at(1), 20);
    CHECK_EQ(contents(sequence), std::vector<int>{10, 30, 40});
    CHECK_EQ(sequence.delete_at(0), 10);
    CHECK_EQ(sequence.delete_at(1), 40);
    CHECK_EQ(contents(sequence), std::vector<int>{30});
}

TEST(2, "first and last operations") {
    ArraySequence<int> sequence;
    sequence.insert_last(2);
    sequence.insert_first(1);
    sequence.insert_last(3);
    CHECK_EQ(contents(sequence), std::vector<int>{1, 2, 3});
    CHECK_EQ(sequence.delete_first(), 1);
    CHECK_EQ(sequence.delete_last(), 3);
    CHECK_EQ(sequence.delete_last(), 2);
    CHECK_EQ(sequence.size(), 0);
}

TEST(2, "invalid indexes throw out_of_range and change nothing") {
    auto sequence = make_sequence({1, 2});
    CHECK_THROWS_AS(sequence.insert_at(3, 9), std::out_of_range);
    CHECK_THROWS_AS(sequence.delete_at(2), std::out_of_range);
    CHECK_THROWS_AS(sequence.get_at(2), std::out_of_range);
    CHECK_THROWS_AS(sequence.set_at(2, 9), std::out_of_range);
    CHECK_EQ(contents(sequence), std::vector<int>{1, 2});

    ArraySequence<int> empty;
    CHECK_THROWS_AS(empty.delete_first(), std::out_of_range);
    CHECK_THROWS_AS(empty.delete_last(), std::out_of_range);
    CHECK_EQ(empty.size(), 0);
}

TEST(2, "copies are deep and moves transfer ownership") {
    auto original = make_sequence({1, 2, 3});
    ArraySequence<int> copy = original;
    copy.set_at(0, 9);
    CHECK_EQ(contents(original), std::vector<int>{1, 2, 3});

    ArraySequence<int> moved = std::move(copy);
    CHECK_EQ(contents(moved), std::vector<int>{9, 2, 3});
    CHECK_EQ(copy.size(), 0);  // NOLINT

    original = moved;
    moved.insert_first(0);
    CHECK_EQ(contents(original), std::vector<int>{9, 2, 3});

    original = std::move(moved);
    CHECK_EQ(contents(original), std::vector<int>{0, 9, 2, 3});
}

TEST(2, "operations leak nothing") {
    const auto before = Tracked::live();
    {
        ArraySequence<Tracked> sequence;
        for (int i = 0; i < 20; ++i) {
            sequence.insert_at(sequence.size() / 2, Tracked(i));
        }
        for (int i = 0; i < 10; ++i) {
            sequence.delete_at(sequence.size() / 2);
        }
        ArraySequence<Tracked> copy = sequence;
        copy.insert_first(Tracked(99));
        CHECK_EQ(copy.size(), 11);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(2, "a throwing copy during insert_at or delete_at leaves the sequence unchanged") {
    const auto before = Tracked::live();
    {
        std::vector<Bomb> items;
        for (int i = 1; i <= 4; ++i) {
            items.emplace_back(i);
        }
        ArraySequence<Bomb> sequence(items);

        Bomb::arm(2);
        CHECK_THROWS_AS(sequence.insert_at(0, Bomb(9)), std::runtime_error);
        Bomb::disarm();
        CHECK_EQ(values_of(sequence), std::vector<int>{1, 2, 3, 4});

        Bomb::arm(2);
        CHECK_THROWS_AS(sequence.delete_at(0), std::runtime_error);
        Bomb::disarm();
        CHECK_EQ(values_of(sequence), std::vector<int>{1, 2, 3, 4});
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(2, "each operation writes at most about n elements") {
    using Item = Probe<int>;
    ArraySequence<Item> sequence;
    const int n = 200;
    for (int i = 0; i < n; ++i) {
        const auto size = sequence.size();
        Item::reset();
        sequence.insert_at(0, Item(i));
        CHECK_LE(Item::writes(), size + 4);
    }
    for (int i = 0; i < n; ++i) {
        const auto size = sequence.size();
        Item::reset();
        sequence.delete_at(size / 2);
        CHECK_LE(Item::writes(), size + 4);
    }
}

TEST(2, "random operations match std::vector") {
    Rng rng;
    ArraySequence<int> sequence;
    std::vector<int> model;
    for (int step = 0; step < 4'000; ++step) {
        INFO("step ", step);
        const auto op = rng.uniform(0, 3);
        const int value = static_cast<int>(rng.uniform(0, 1'000'000));
        if (op == 0 || model.empty()) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size())));
            sequence.insert_at(i, value);
            model.insert(model.begin() + static_cast<std::ptrdiff_t>(i), value);
        } else if (op == 1) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            const int removed = sequence.delete_at(i);
            REQUIRE_EQ(removed, model[i]);
            model.erase(model.begin() + static_cast<std::ptrdiff_t>(i));
        } else if (op == 2) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            sequence.set_at(i, value);
            model[i] = value;
        } else {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            REQUIRE_EQ(sequence.get_at(i), model[i]);
        }
        REQUIRE_EQ(sequence.size(), model.size());
    }
    CHECK_EQ(contents(sequence), model);
}

TEST(3, "no two records share a key") {
    const std::vector<int> records = {1, 2, 3};
    CHECK(!first_collision<int>(records, [](int r) { return r; }).has_value());
    const std::vector<int> none;
    CHECK(!first_collision<int>(none, [](int r) { return r; }).has_value());
    const std::vector<int> one = {5};
    CHECK(!first_collision<int>(one, [](int r) { return r; }).has_value());
}

TEST(3, "the first collision is reported with the earliest match") {
    const std::vector<int> records = {4, 7, 9, 7, 4, 7};
    const auto result = first_collision<int>(records, [](int r) { return r; });
    REQUIRE(result.has_value());
    CHECK_EQ(result->first, 1);
    CHECK_EQ(result->second, 3);
}

TEST(3, "keys are extracted from records") {
    struct Student {
        std::string name;
        int birthday;
    };
    const std::vector<Student> students = {{"ada", 12}, {"bob", 30}, {"cleo", 12}};
    const auto result = first_collision<Student>(students, [](const Student& s) { return s.birthday; });
    REQUIRE(result.has_value());
    CHECK_EQ(students[result->first].name, "ada");
    CHECK_EQ(students[result->second].name, "cleo");
}

TEST(3, "random records match a double loop") {
    Rng rng;
    for (int round = 0; round < 2'000; ++round) {
        const auto records = rng.ints(static_cast<std::size_t>(rng.uniform(0, 30)), 0, 40);
        INFO("round ", round);
        std::optional<std::pair<std::size_t, std::size_t>> expected;
        for (std::size_t j = 0; j < records.size() && !expected; ++j) {
            for (std::size_t i = 0; i < j; ++i) {
                if (records[i] == records[j]) {
                    expected = std::pair(i, j);
                    break;
                }
            }
        }
        const auto actual = first_collision<int>(records, [](int r) { return r; });
        REQUIRE_EQ(actual.has_value(), expected.has_value());
        if (expected) {
            REQUIRE_EQ(actual->first, expected->first);
            REQUIRE_EQ(actual->second, expected->second);
        }
    }
}
