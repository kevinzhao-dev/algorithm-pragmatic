#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "00_alloc_counter.hpp"
#include "00_counters.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;
using practice6006::AllocationScope;
using practice6006::Probe;

namespace {

struct NoDefault {
    explicit NoDefault(int v) : value(v) {}
    int value;
};

// Copies and moves can throw when armed. A move leaves the source zeroed, so a container that
// moves elements during a growth that then fails is caught by the contents check.
class Hazard {
public:
    explicit Hazard(int value) : value_(value) { ++live(); }
    Hazard(const Hazard& other) : value_(other.value_) {
        tick();
        ++live();
    }
    Hazard(Hazard&& other) : value_(other.value_) {  // deliberately not noexcept
        tick();
        other.value_ = -1;
        ++live();
    }
    Hazard& operator=(const Hazard& other) {
        tick();
        value_ = other.value_;
        return *this;
    }
    Hazard& operator=(Hazard&& other) {
        tick();
        value_ = other.value_;
        other.value_ = -1;
        return *this;
    }
    ~Hazard() { --live(); }

    int value() const { return value_; }

    // The next `operations` copies or moves succeed; the one after that throws.
    static void arm(int operations) { countdown() = operations; }
    static void disarm() { countdown() = -1; }
    static std::int64_t& live() {
        static std::int64_t value = 0;
        return value;
    }

private:
    static void tick() {
        int& left = countdown();
        if (left == 0) {
            left = -1;
            throw std::runtime_error("hazard");
        }
        if (left > 0) {
            --left;
        }
    }
    static int& countdown() {
        static int value = -1;
        return value;
    }
    int value_;
};

template <typename T>
std::vector<int> values_of(const DynamicArray<T>& array) {
    std::vector<int> out;
    for (const auto& item : array) {
        if constexpr (requires { item.value(); }) {
            out.push_back(item.value());
        } else {
            out.push_back(item);
        }
    }
    return out;
}

DynamicArray<int> make_array(const std::vector<int>& values) {
    DynamicArray<int> array;
    for (int value : values) {
        array.insert_last(value);
    }
    return array;
}

template <typename T>
void check_invariants(const DynamicArray<T>& array) {
    CHECK_LE(array.size(), array.capacity());
    CHECK_LE(array.capacity(), 4 * array.size() + 16);
}

}  // namespace

TEST(1, "a default-constructed array is empty and allocates nothing") {
    const AllocationScope scope;
    DynamicArray<int> array;
    CHECK_EQ(scope.allocations(), 0);
    CHECK(array.empty());
    CHECK_EQ(array.size(), 0);
    CHECK_EQ(array.capacity(), 0);
    CHECK(array.begin() == array.end());
}

TEST(1, "insert_last appends in order") {
    DynamicArray<int> array;
    for (int i = 0; i < 100; ++i) {
        array.insert_last(i * 3);
    }
    CHECK_EQ(array.size(), 100);
    for (std::size_t i = 0; i < array.size(); ++i) {
        CHECK_EQ(array[i], static_cast<int>(i) * 3);
        CHECK_EQ(array.get_at(i), static_cast<int>(i) * 3);
    }
}

TEST(1, "delete_last returns the last item") {
    auto array = make_array({1, 2, 3});
    CHECK_EQ(array.delete_last(), 3);
    CHECK_EQ(array.delete_last(), 2);
    CHECK_EQ(array.size(), 1);
    CHECK_EQ(array[0], 1);
}

TEST(1, "deleting from an empty array throws out_of_range") {
    DynamicArray<int> array;
    CHECK_THROWS_AS(array.delete_last(), std::out_of_range);
    array.insert_last(1);
    array.delete_last();
    CHECK_THROWS_AS(array.delete_last(), std::out_of_range);
}

TEST(1, "checked access throws out_of_range and changes nothing") {
    auto array = make_array({1, 2});
    CHECK_THROWS_AS(array.get_at(2), std::out_of_range);
    CHECK_THROWS_AS(array.set_at(2, 9), std::out_of_range);
    array.set_at(1, 7);
    CHECK_EQ(values_of(array), std::vector<int>{1, 7});
}

TEST(1, "iteration visits exactly size() elements") {
    auto array = make_array({4, 5, 6});
    array.delete_last();
    int sum = 0;
    for (int value : array) {
        sum += value;
    }
    CHECK_EQ(sum, 9);
    const DynamicArray<int>& view = array;
    CHECK_EQ(*view.begin(), 4);
    CHECK_EQ(view.end() - view.begin(), 2);
}

TEST(1, "element types need no default constructor") {
    DynamicArray<NoDefault> array;
    array.insert_last(NoDefault(1));
    array.insert_last(NoDefault(2));
    CHECK_EQ(array.delete_last().value, 2);
    CHECK_EQ(array.get_at(0).value, 1);
}

TEST(1, "move-only elements work") {
    DynamicArray<std::unique_ptr<int>> array;
    for (int i = 0; i < 40; ++i) {
        array.insert_last(std::make_unique<int>(i));
    }
    const std::unique_ptr<int> last = array.delete_last();
    CHECK_EQ(*last, 39);
    CHECK_EQ(*array[20], 20);
}

TEST(1, "strings survive reallocation") {
    DynamicArray<std::string> array;
    for (int i = 0; i < 200; ++i) {
        array.insert_last(std::string(40, static_cast<char>('a' + i % 26)));
    }
    CHECK_EQ(array[199], std::string(40, static_cast<char>('a' + 199 % 26)));
    CHECK_EQ(array.size(), 200);
}

TEST(1, "spare capacity holds no constructed elements") {
    const auto before = Tracked::live();
    {
        DynamicArray<Tracked> array;
        for (int i = 0; i < 200; ++i) {
            array.insert_last(Tracked(i));
            INFO("after insert ", i);
            REQUIRE_EQ(Tracked::live() - before, array.size());
        }
        for (int i = 0; i < 150; ++i) {
            CHECK_EQ(array.delete_last().value(), 199 - i);
            INFO("after delete ", i);
            REQUIRE_EQ(Tracked::live() - before, array.size());
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "random operations match std::vector") {
    Rng rng;
    DynamicArray<int> array;
    std::vector<int> model;
    for (int step = 0; step < 30'000; ++step) {
        INFO("step ", step);
        const int value = static_cast<int>(rng.uniform(0, 1'000'000));
        if (rng.coin(0.55) || model.empty()) {
            array.insert_last(value);
            model.push_back(value);
        } else {
            REQUIRE_EQ(array.delete_last(), model.back());
            model.pop_back();
        }
        REQUIRE_EQ(array.size(), model.size());
        if (!model.empty()) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            REQUIRE_EQ(array.get_at(i), model[i]);
        }
    }
    CHECK_EQ(values_of(array), model);
}

TEST(2, "capacity stays proportional to size") {
    Rng rng;
    DynamicArray<int> array;
    for (int step = 0; step < 50'000; ++step) {
        // A random walk that repeatedly grows and then drains the array.
        const bool grow = (step / 5'000) % 2 == 0 ? rng.coin(0.7) : rng.coin(0.3);
        if (grow || array.empty()) {
            array.insert_last(step);
        } else {
            array.delete_last();
        }
        INFO("step ", step);
        check_invariants(array);
    }
}

TEST(2, "n insertions cost at most 5n element writes") {
    using Item = Probe<int>;
    for (int n : {1, 2, 3, 7, 8, 9, 1'000, 1'024, 1'025, 100'000}) {
        Item::reset();
        DynamicArray<Item> array;
        for (int i = 0; i < n; ++i) {
            array.insert_last(Item(i));
        }
        INFO("n=", n);
        CHECK_LE(Item::writes(), static_cast<std::uint64_t>(5 * n));
    }
}

TEST(2, "alternating at a full boundary does not thrash") {
    using Item = Probe<int>;
    DynamicArray<Item> array;
    int next = 0;
    while (array.size() < 2'000 || array.size() != array.capacity()) {
        array.insert_last(Item(next++));
    }
    const std::size_t rounds = 10'000;
    Item::reset();
    for (std::size_t round = 0; round < rounds; ++round) {
        array.insert_last(Item(1));
        CHECK_EQ(array.delete_last().value(), 1);
    }
    // One growth relocates the whole array; after that each round is constant work.
    CHECK_LE(Item::writes(), 8 * rounds + 3 * array.capacity());
}

TEST(2, "alternating at a shrink boundary does not thrash") {
    using Item = Probe<int>;
    DynamicArray<Item> array;
    for (int i = 0; i < 4'000; ++i) {
        array.insert_last(Item(i));
    }
    // Drain step by step; at every size, a burst of alternations must stay cheap in total.
    std::uint64_t worst = 0;
    while (array.size() > 16) {
        Item::reset();
        for (int round = 0; round < 8; ++round) {
            array.delete_last();
            array.insert_last(Item(7));
        }
        worst = std::max(worst, Item::writes());
        array.delete_last();
    }
    CHECK_LE(worst, 8 * 8 + 3 * 4'000);
}

TEST(2, "a long mixed workload does amortized constant writes") {
    using Item = Probe<int>;
    Rng rng;
    DynamicArray<Item> array;
    Item::reset();
    std::size_t operations = 0;
    for (int phase = 0; phase < 60; ++phase) {
        const auto target = static_cast<std::size_t>(rng.uniform(0, 3'000));
        while (array.size() < target) {
            array.insert_last(Item(1));
            ++operations;
        }
        while (array.size() > target) {
            array.delete_last();
            ++operations;
        }
    }
    CHECK_LE(Item::writes(), 9 * operations);
}

PERF(2, "tens of millions of appends and pops", 3000) {
    DynamicArray<std::int64_t> array;
    for (std::int64_t i = 0; i < 30'000'000; ++i) {
        array.insert_last(i);
    }
    std::int64_t sum = 0;
    while (!array.empty()) {
        sum += array.delete_last();
    }
    CHECK_GT(sum, 0);
}

TEST(3, "copy construction is deep") {
    auto a = make_array({1, 2, 3});
    DynamicArray<int> b(a);
    b.set_at(0, 9);
    b.insert_last(4);
    CHECK_EQ(values_of(a), std::vector<int>{1, 2, 3});
    CHECK_EQ(values_of(b), std::vector<int>{9, 2, 3, 4});
}

TEST(3, "copying an empty array") {
    DynamicArray<int> empty;
    DynamicArray<int> copy(empty);
    CHECK(copy.empty());
    copy.insert_last(1);
    CHECK(empty.empty());
}

TEST(3, "copy assignment replaces contents and tolerates self-assignment") {
    auto a = make_array({1, 2});
    auto b = make_array({7, 8, 9});
    b = a;
    CHECK_EQ(values_of(b), std::vector<int>{1, 2});
    b.insert_last(3);
    CHECK_EQ(values_of(a), std::vector<int>{1, 2});
    a = *&a;
    CHECK_EQ(values_of(a), std::vector<int>{1, 2});
}

TEST(3, "move operations steal the buffer") {
    using Item = Probe<int>;
    DynamicArray<Item> a;
    for (int i = 0; i < 10; ++i) {
        a.insert_last(Item(i));
    }
    Item::reset();
    DynamicArray<Item> b(std::move(a));
    CHECK_EQ(Item::writes(), 0);
    CHECK(a.empty());  // NOLINT: moved-from state is part of this class's contract
    CHECK_EQ(a.capacity(), 0);  // NOLINT
    CHECK_EQ(b.size(), 10);

    DynamicArray<Item> c;
    c.insert_last(Item(99));
    Item::reset();
    c = std::move(b);
    CHECK_EQ(Item::writes(), 0);
    CHECK(b.empty());  // NOLINT
    CHECK_EQ(c.size(), 10);
    CHECK_EQ(c[3].value(), 3);

    // Moved-from objects are still assignable and reusable.
    a = DynamicArray<Item>();
    a.insert_last(Item(1));
    CHECK_EQ(a.size(), 1);
}

TEST(3, "moves are noexcept") {
    DynamicArray<int> array;  // reports "not implemented" until the class exists
    CHECK(std::is_nothrow_move_constructible_v<DynamicArray<int>>);
    CHECK(std::is_nothrow_move_assignable_v<DynamicArray<int>>);
}

TEST(3, "copies, moves and assignments leak nothing") {
    const auto before = Tracked::live();
    {
        DynamicArray<Tracked> a;
        for (int i = 0; i < 30; ++i) {
            a.insert_last(Tracked(i));
        }
        DynamicArray<Tracked> b(a);
        DynamicArray<Tracked> c;
        c = b;
        DynamicArray<Tracked> d(std::move(a));
        d = std::move(c);
        b = d;
        CHECK_EQ(b.size(), 30);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(3, "a failed copy leaves the source intact and leaks nothing") {
    const auto before = Hazard::live();
    {
        DynamicArray<Hazard> a;
        for (int i = 1; i <= 5; ++i) {
            a.insert_last(Hazard(i));
        }
        Hazard::arm(2);
        CHECK_THROWS_AS(DynamicArray<Hazard>(a), std::runtime_error);
        Hazard::disarm();
        CHECK_EQ(values_of(a), std::vector<int>{1, 2, 3, 4, 5});

        DynamicArray<Hazard> target;
        target.insert_last(Hazard(100));
        Hazard::arm(3);
        CHECK_THROWS_AS(target = a, std::runtime_error);
        Hazard::disarm();
        CHECK_EQ(values_of(target), std::vector<int>{100});
    }
    CHECK_EQ(Hazard::live(), before);
}

TEST(3, "insert_at at the front, middle and end") {
    auto array = make_array({10, 20, 30});
    array.insert_at(0, 5);
    array.insert_at(2, 15);
    array.insert_at(array.size(), 35);
    CHECK_EQ(values_of(array), std::vector<int>{5, 10, 15, 20, 30, 35});
}

TEST(3, "delete_at at the front, middle and end") {
    auto array = make_array({1, 2, 3, 4, 5});
    CHECK_EQ(array.delete_at(0), 1);
    CHECK_EQ(array.delete_at(1), 3);
    CHECK_EQ(array.delete_at(array.size() - 1), 5);
    CHECK_EQ(values_of(array), std::vector<int>{2, 4});
}

TEST(3, "invalid positions throw and change nothing") {
    auto array = make_array({1, 2});
    CHECK_THROWS_AS(array.insert_at(3, 9), std::out_of_range);
    CHECK_THROWS_AS(array.delete_at(2), std::out_of_range);
    CHECK_EQ(values_of(array), std::vector<int>{1, 2});
}

TEST(3, "insert_at into a full array still works") {
    DynamicArray<int> array;
    int next = 0;
    while (array.size() < 5 || array.size() != array.capacity()) {
        array.insert_last(next++);
    }
    array.insert_at(1, 100);
    CHECK_EQ(array[1], 100);
    CHECK_EQ(array[0], 0);
    CHECK_EQ(array[2], 1);
}

TEST(3, "shifting moves only the elements after the position") {
    using Item = Probe<int>;
    DynamicArray<Item> array;
    const int n = 1'000;
    for (int i = 0; i < n; ++i) {
        array.insert_last(Item(i));
    }
    // Guarantee spare room: one extra push forces a growth if the array is exactly full.
    array.insert_last(Item(0));
    array.delete_last();
    REQUIRE(array.size() < array.capacity());

    const std::size_t position = array.size() - 50;
    Item::reset();
    array.insert_at(position, Item(-1));
    CHECK_LE(Item::writes(), 50 + 8);

    Item::reset();
    CHECK_EQ(array.delete_at(position).value(), -1);
    CHECK_LE(Item::writes(), 50 + 8);
}

TEST(3, "random operations including the middle match std::vector") {
    Rng rng;
    DynamicArray<int> array;
    std::vector<int> model;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const auto op = rng.uniform(0, 4);
        const int value = static_cast<int>(rng.uniform(0, 1'000'000));
        const auto bound = static_cast<std::int64_t>(model.size());
        if (op == 0 || model.empty()) {
            array.insert_last(value);
            model.push_back(value);
        } else if (op == 1) {
            REQUIRE_EQ(array.delete_last(), model.back());
            model.pop_back();
        } else if (op == 2) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, bound));
            array.insert_at(i, value);
            model.insert(model.begin() + static_cast<std::ptrdiff_t>(i), value);
        } else if (op == 3) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, bound - 1));
            REQUIRE_EQ(array.delete_at(i), model[i]);
            model.erase(model.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            const auto i = static_cast<std::size_t>(rng.uniform(0, bound - 1));
            array.set_at(i, value);
            model[i] = value;
        }
        REQUIRE_EQ(array.size(), model.size());
        check_invariants(array);
    }
    CHECK_EQ(values_of(array), model);
}

TEST(4, "growth moves elements when moves are noexcept") {
    using Item = Probe<int>;
    Item::reset();
    DynamicArray<Item> array;
    for (int i = 0; i < 2'000; ++i) {
        array.insert_last(Item(i));
    }
    CHECK_EQ(Item::copies(), 0);
    CHECK_GT(Item::moves(), 2'000);
}

TEST(4, "a throw anywhere during growth leaves the array untouched") {
    const auto before = Hazard::live();
    for (int arm = 0; arm < 64; ++arm) {
        DynamicArray<Hazard> array;
        int next = 1;
        while (array.size() < 5 || array.size() != array.capacity()) {
            array.insert_last(Hazard(next++));
        }
        const std::size_t size = array.size();
        std::vector<int> expected;
        for (int i = 1; i < next; ++i) {
            expected.push_back(i);
        }

        INFO("arm=", arm);
        Hazard::arm(arm);
        bool threw = false;
        try {
            array.insert_last(Hazard(99));
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Hazard::disarm();

        if (threw) {
            REQUIRE_EQ(array.size(), size);
            REQUIRE_EQ(values_of(array), expected);
        } else {
            expected.push_back(99);
            REQUIRE_EQ(values_of(array), expected);
        }
    }
    CHECK_EQ(Hazard::live(), before);
}

TEST(4, "after a failed growth the array is fully usable") {
    const auto before = Hazard::live();
    {
        DynamicArray<Hazard> array;
        int next = 1;
        while (array.size() < 3 || array.size() != array.capacity()) {
            array.insert_last(Hazard(next++));
        }
        Hazard::arm(1);
        CHECK_THROWS_AS(array.insert_last(Hazard(50)), std::runtime_error);
        Hazard::disarm();
        array.insert_last(Hazard(51));
        CHECK_EQ(array.get_at(array.size() - 1).value(), 51);
        CHECK_EQ(array.delete_last().value(), 51);
    }
    CHECK_EQ(Hazard::live(), before);
}
