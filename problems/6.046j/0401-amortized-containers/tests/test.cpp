#include <bit>
#include <deque>
#include <limits>
#include <set>
#include <stdexcept>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;
using practice::Tracked;

namespace {

// Counts every element-moving operation (copies and moves alike) and allows no default state.
class MoveCount {
public:
    explicit MoveCount(int value) : value_(value) {}
    MoveCount(const MoveCount& other) : value_(other.value_) { ++count(); }
    MoveCount(MoveCount&& other) noexcept : value_(other.value_) { ++count(); }
    MoveCount& operator=(const MoveCount&) = delete;
    MoveCount& operator=(MoveCount&&) = delete;

    int value() const { return value_; }

    static std::uint64_t& count() {
        static std::uint64_t total = 0;
        return total;
    }

private:
    int value_;
};

// Its move can throw, so reallocation has to copy; and copies can be made to fail.
class Fragile {
public:
    explicit Fragile(int value) : tracked_(value) {}
    Fragile(const Fragile& other) : tracked_(other.tracked_.value()) {
        if (copies_left()-- == 0) {
            throw std::runtime_error("copy failed");
        }
    }
    Fragile(Fragile&& other) : Fragile(static_cast<const Fragile&>(other)) {}
    Fragile& operator=(const Fragile&) = delete;

    int value() const { return tracked_.value(); }

    static int& copies_left() {
        static int left = std::numeric_limits<int>::max();
        return left;
    }

private:
    Tracked tracked_;
};

void fill(DynamicArray<int>& array, int count) {
    for (int i = 0; i < count; ++i) {
        array.push_back(i);
    }
}

}  // namespace

TEST(1, "a new array is empty and owns no storage") {
    const DynamicArray<int> array;
    CHECK_EQ(array.size(), 0);
    CHECK_EQ(array.capacity(), 0);
    CHECK(array.empty());
}

TEST(1, "capacity doubles exactly when the table is full") {
    DynamicArray<int> array;
    const std::vector<std::size_t> expected{1, 2, 4, 4, 8, 8, 8, 8, 16};
    for (std::size_t i = 0; i < expected.size(); ++i) {
        array.push_back(static_cast<int>(i));
        CHECK_EQ(array.capacity(), expected[i]);
        CHECK_EQ(array.size(), i + 1);
    }
}

TEST(1, "elements stay readable and writable through growth") {
    DynamicArray<int> array;
    fill(array, 1000);
    array[500] = 7;
    for (int i = 0; i < 1000; ++i) {
        CHECK_EQ(array[static_cast<std::size_t>(i)], i == 500 ? 7 : i);
    }
    const auto& constant = array;
    CHECK_EQ(constant[0], 0);
}

TEST(1, "an element type without a default constructor works") {
    DynamicArray<MoveCount> array;
    array.push_back(MoveCount(4));
    array.push_back(MoveCount(5));
    CHECK_EQ(array[1].value(), 5);
}

TEST(1, "pushing an element of the same array is safe") {
    DynamicArray<int> array;
    array.push_back(1);
    for (int i = 0; i < 40; ++i) {
        array.push_back(array[0] + i);
    }
    CHECK_EQ(array[40], 40);
}

TEST(1, "growth moves each element a constant number of times on average") {
    MoveCount::count() = 0;
    DynamicArray<MoveCount> array;
    const std::uint64_t n = 1 << 18;
    for (std::uint64_t i = 0; i < n; ++i) {
        array.push_back(MoveCount(static_cast<int>(i)));
    }
    // One move into the array per push, plus at most one full table per doubling.
    CHECK_LE(MoveCount::count(), 4 * n);
    for (std::uint64_t i = 0; i < n; ++i) {
        CHECK_EQ(array[i].value(), static_cast<int>(i));
    }
}

TEST(1, "no element leaks or is destroyed twice") {
    const auto before = Tracked::live();
    {
        DynamicArray<Tracked> array;
        for (int i = 0; i < 2000; ++i) {
            array.push_back(Tracked(i));
        }
        DynamicArray<Tracked> copy(array);
        CHECK_EQ(Tracked::live() - before, 4000);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "copying makes an independent array") {
    DynamicArray<int> array;
    fill(array, 10);
    DynamicArray<int> copy(array);
    copy[0] = 99;
    copy.push_back(5);
    CHECK_EQ(array[0], 0);
    CHECK_EQ(array.size(), 10);
    CHECK_EQ(copy.size(), 11);
}

TEST(1, "moving transfers the storage and leaves the source empty") {
    DynamicArray<int> array;
    fill(array, 10);
    DynamicArray<int> moved(std::move(array));
    CHECK_EQ(moved.size(), 10);
    CHECK_EQ(moved[9], 9);
    CHECK_EQ(array.size(), 0);
    CHECK_EQ(array.capacity(), 0);
    array.push_back(1);
    CHECK_EQ(array[0], 1);
}

TEST(1, "assignment replaces the old contents") {
    DynamicArray<int> a;
    DynamicArray<int> b;
    fill(a, 3);
    fill(b, 5);
    a = b;
    CHECK_EQ(a.size(), 5);
    a[0] = 100;
    CHECK_EQ(b[0], 0);
    b = std::move(a);
    CHECK_EQ(b.size(), 5);
    CHECK_EQ(b[0], 100);
}

TEST(1, "self assignment is harmless") {
    DynamicArray<int> array;
    fill(array, 4);
    auto& alias = array;
    array = alias;
    CHECK_EQ(array.size(), 4);
    CHECK_EQ(array[3], 3);
}

TEST(1, "a failed copy during growth leaves the array untouched") {
    const auto before = Tracked::live();
    {
        DynamicArray<Fragile> array;
        for (int i = 0; i < 4; ++i) {
            array.push_back(Fragile(i));
        }
        REQUIRE_EQ(array.capacity(), 4);
        Fragile::copies_left() = 2;
        CHECK_THROWS_AS(array.push_back(Fragile(9)), std::runtime_error);
        Fragile::copies_left() = std::numeric_limits<int>::max();
        CHECK_EQ(array.size(), 4);
        CHECK_EQ(array.capacity(), 4);
        for (int i = 0; i < 4; ++i) {
            CHECK_EQ(array[static_cast<std::size_t>(i)].value(), i);
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "a failed copy construction does not leak") {
    const auto before = Tracked::live();
    {
        DynamicArray<Fragile> array;
        for (int i = 0; i < 4; ++i) {
            array.push_back(Fragile(i));
        }
        Fragile::copies_left() = 2;
        CHECK_THROWS_AS(DynamicArray<Fragile>{array}, std::runtime_error);
        Fragile::copies_left() = std::numeric_limits<int>::max();
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "random pushes match std::vector") {
    Rng rng;
    DynamicArray<int> array;
    std::vector<int> model;
    for (int step = 0; step < 20'000; ++step) {
        const int value = static_cast<int>(rng.uniform(0, 1'000'000));
        array.push_back(value);
        model.push_back(value);
    }
    for (std::size_t i = 0; i < model.size(); ++i) {
        CHECK_EQ(array[i], model[i]);
    }
}

TEST(2, "popping an empty array throws") {
    DynamicArray<int> array;
    CHECK_THROWS_AS(array.pop_back(), std::out_of_range);
}

TEST(2, "pops shrink the table at a quarter full") {
    DynamicArray<int> array;
    fill(array, 16);
    CHECK_EQ(array.capacity(), 16);
    for (int i = 0; i < 11; ++i) {
        array.pop_back();
    }
    CHECK_EQ(array.size(), 5);
    CHECK_EQ(array.capacity(), 16);
    array.pop_back();
    CHECK_EQ(array.size(), 4);
    CHECK_EQ(array.capacity(), 8);
}

TEST(2, "popping everything releases the storage") {
    DynamicArray<int> array;
    fill(array, 8);
    for (int i = 0; i < 8; ++i) {
        array.pop_back();
    }
    CHECK_EQ(array.size(), 0);
    CHECK_EQ(array.capacity(), 0);
    array.push_back(1);
    CHECK_EQ(array.capacity(), 1);
}

TEST(2, "elements survive shrinking") {
    DynamicArray<int> array;
    fill(array, 100);
    for (int i = 0; i < 90; ++i) {
        array.pop_back();
    }
    for (int i = 0; i < 10; ++i) {
        CHECK_EQ(array[static_cast<std::size_t>(i)], i);
    }
}

TEST(2, "the table never leaves the quarter-to-full range") {
    Rng rng;
    DynamicArray<int> array;
    for (int step = 0; step < 50'000; ++step) {
        if (array.empty() || rng.coin(0.5)) {
            array.push_back(step);
        } else {
            array.pop_back();
        }
        if (array.size() > 1) {
            CHECK_LE(array.size(), array.capacity());
            CHECK_GT(4 * array.size(), array.capacity());
        }
    }
}

TEST(2, "oscillating at a resize boundary stays amortized constant") {
    MoveCount::count() = 0;
    DynamicArray<MoveCount> array;
    const int n = (1 << 12) + 1;
    for (int i = 0; i < n; ++i) {
        array.push_back(MoveCount(i));
    }
    REQUIRE_EQ(array.capacity(), 1 << 13);
    const auto base = MoveCount::count();
    for (int round = 0; round < 20'000; ++round) {
        array.pop_back();
        array.push_back(MoveCount(round));
    }
    CHECK_LE(MoveCount::count() - base, 60'000);
}

TEST(2, "a fast push and pop mix moves a linear number of elements") {
    MoveCount::count() = 0;
    Rng rng;
    DynamicArray<MoveCount> array;
    const std::uint64_t operations = 200'000;
    for (std::uint64_t step = 0; step < operations; ++step) {
        if (array.empty() || rng.coin(0.6)) {
            array.push_back(MoveCount(0));
        } else {
            array.pop_back();
        }
    }
    CHECK_LE(MoveCount::count(), 8 * operations);
}

TEST(2, "a failed copy while shrinking keeps the old table and the contents") {
    const auto before = Tracked::live();
    {
        DynamicArray<Fragile> array;
        for (int i = 0; i < 8; ++i) {
            array.push_back(Fragile(i));
        }
        for (int i = 0; i < 5; ++i) {
            array.pop_back();
        }
        REQUIRE_EQ(array.capacity(), 8);
        Fragile::copies_left() = 0;
        array.pop_back();
        Fragile::copies_left() = std::numeric_limits<int>::max();
        CHECK_EQ(array.size(), 2);
        CHECK_EQ(array.capacity(), 8);
        CHECK_EQ(array[1].value(), 1);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(3, "an empty index holds nothing") {
    const SortedRuns<int> index;
    CHECK_EQ(index.size(), 0);
    CHECK_EQ(index.run_count(), 0);
    CHECK(!index.contains(1));
    CHECK(index.sorted().empty());
}

TEST(3, "values can be found after insertion") {
    SortedRuns<int> index;
    for (int value : {5, 3, 9, 1}) {
        index.insert(value);
    }
    CHECK(index.contains(3));
    CHECK(!index.contains(4));
    CHECK_EQ(index.sorted(), (std::vector<int>{1, 3, 5, 9}));
}

TEST(3, "the number of runs is the bit count of the size") {
    Rng rng;
    SortedRuns<int> index;
    for (std::size_t n = 1; n <= 2000; ++n) {
        index.insert(static_cast<int>(rng.uniform(0, 100)));
        CHECK_EQ(index.run_count(), static_cast<std::size_t>(std::popcount(n)));
    }
}

TEST(3, "duplicates are kept") {
    SortedRuns<int> index;
    for (int i = 0; i < 4; ++i) {
        index.insert(7);
    }
    CHECK_EQ(index.size(), 4);
    CHECK_EQ(index.sorted(), (std::vector<int>{7, 7, 7, 7}));
}

TEST(3, "inserts cost O(log n) comparisons amortized") {
    Rng rng;
    Counted<int>::comparisons() = 0;
    SortedRuns<Counted<int>> index;
    const std::uint64_t n = 1 << 14;
    for (int value : rng.permutation(n)) {
        index.insert(Counted<int>(value));
    }
    CHECK_LE(Counted<int>::comparisons(), n * 16);
}

TEST(3, "a lookup touches each run with a binary search") {
    Rng rng;
    SortedRuns<Counted<int>> index;
    const std::uint64_t n = (1 << 14) - 1;
    for (int value : rng.permutation(n)) {
        index.insert(Counted<int>(value));
    }
    Counted<int>::comparisons() = 0;
    int found = 0;
    for (int value = 0; value < 2000; ++value) {
        found += index.contains(Counted<int>(value));
    }
    CHECK_EQ(found, 2000);
    CHECK_LE(Counted<int>::comparisons(), 2000ULL * 14 * 16);
}

TEST(3, "random inserts match a multiset") {
    Rng rng;
    SortedRuns<int> index;
    std::multiset<int> model;
    for (int step = 0; step < 3000; ++step) {
        const int value = static_cast<int>(rng.uniform(0, 500));
        index.insert(value);
        model.insert(value);
        const int probe = static_cast<int>(rng.uniform(0, 500));
        REQUIRE_EQ(index.contains(probe), model.contains(probe));
    }
    CHECK_EQ(index.sorted(), std::vector<int>(model.begin(), model.end()));
}

TEST(3, "no element leaks from the merges") {
    const auto before = Tracked::live();
    {
        struct Key {
            Tracked value;
            bool operator<(const Key& other) const { return value.value() < other.value.value(); }
        };
        SortedRuns<Key> index;
        for (int i = 0; i < 1000; ++i) {
            index.insert(Key{Tracked(i)});
        }
    }
    CHECK_EQ(Tracked::live(), before);
}
