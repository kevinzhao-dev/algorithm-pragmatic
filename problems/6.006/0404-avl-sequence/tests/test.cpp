#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

struct SumAug {
    using Value = long long;
    static Value identity() { return 0; }
    static Value of(int item) { return item; }
    static Value combine(Value a, Value b) { return a + b; }
};

struct ConcatAug {
    using Value = std::string;
    static Value identity() { return {}; }
    static Value of(const std::string& item) { return item; }
    static Value combine(const Value& a, const Value& b) { return a + b; }
};

struct CountingAug {
    using Value = long long;
    static inline std::uint64_t calls = 0;
    static Value identity() { return 0; }
    static Value of(int item) { return item; }
    static Value combine(Value a, Value b) {
        ++calls;
        return a + b;
    }
};

using Seq = AvlSequence<int>;

template <typename S>
void check_shape(const S& seq) {
    const auto* root = seq.tree().root();
    if (root == nullptr) {
        CHECK_EQ(seq.size(), 0);
        return;
    }
    CHECK(root->height <= 1.45 * std::log2(static_cast<double>(seq.size()) + 2));
    CHECK_EQ(root->size, seq.size());
}

}  // namespace

TEST(1, "build keeps order and indexes correctly") {
    const Seq seq(std::vector<int>{4, 8, 15, 16, 23, 42});
    CHECK_EQ(seq.size(), 6);
    CHECK_EQ(seq.to_vector(), std::vector<int>{4, 8, 15, 16, 23, 42});
    CHECK_EQ(seq.at(0), 4);
    CHECK_EQ(seq.at(5), 42);
    CHECK_EQ(seq.node_at(3)->item, 16);
    check_shape(seq);
}

TEST(1, "an empty sequence") {
    Seq seq;
    CHECK(seq.empty());
    CHECK_EQ(seq.size(), 0);
    CHECK(seq.to_vector().empty());
    CHECK_THROWS_AS(seq.at(0), std::out_of_range);
    const Seq built(std::vector<int>{});
    CHECK(built.empty());
}

TEST(1, "set_at replaces one element") {
    Seq seq(std::vector<int>{1, 2, 3});
    seq.set_at(1, 20);
    CHECK_EQ(seq.to_vector(), std::vector<int>{1, 20, 3});
}

TEST(1, "indexing past the end throws") {
    Seq seq(std::vector<int>{1, 2});
    CHECK_THROWS_AS(seq.at(2), std::out_of_range);
    CHECK_THROWS_AS(seq.node_at(99), std::out_of_range);
    CHECK_THROWS_AS(seq.set_at(5, 1), std::out_of_range);
}

TEST(1, "random reads and writes match a vector") {
    Rng rng;
    std::vector<int> model(5000);
    std::iota(model.begin(), model.end(), 0);
    Seq seq(model);
    for (int step = 0; step < 100'000; ++step) {
        INFO("step ", step);
        const auto i = static_cast<std::size_t>(rng.uniform(0, 4999));
        if (rng.coin()) {
            const int value = static_cast<int>(rng.uniform(0, 1'000'000));
            seq.set_at(i, value);
            model[i] = value;
        } else {
            REQUIRE_EQ(seq.at(i), model[i]);
        }
    }
    CHECK_EQ(seq.to_vector(), model);
    check_shape(seq);
}

PERF(1, "indexing a large sequence is logarithmic", 2500) {
    std::vector<int> values(500'000);
    std::iota(values.begin(), values.end(), 0);
    Seq seq(values);
    Rng rng;
    long long sum = 0;
    for (int i = 0; i < 1'000'000; ++i) {
        sum += seq.at(static_cast<std::size_t>(rng.uniform(0, 499'999)));
    }
    CHECK_GT(sum, 0);
}

TEST(2, "insert_at puts the value at that position") {
    Seq seq(std::vector<int>{1, 2, 3});
    seq.insert_at(1, 9);
    CHECK_EQ(seq.to_vector(), std::vector<int>{1, 9, 2, 3});
    seq.insert_at(0, 7);
    seq.insert_at(5, 8);
    CHECK_EQ(seq.to_vector(), std::vector<int>{7, 1, 9, 2, 3, 8});
    CHECK_THROWS_AS(seq.insert_at(7, 0), std::out_of_range);
}

TEST(2, "insert into an empty sequence") {
    Seq seq;
    seq.insert_at(0, 5);
    CHECK_EQ(seq.to_vector(), std::vector<int>{5});
}

TEST(2, "delete_at removes and returns the element") {
    Seq seq(std::vector<int>{1, 2, 3, 4});
    CHECK_EQ(seq.delete_at(1), 2);
    CHECK_EQ(seq.to_vector(), std::vector<int>{1, 3, 4});
    CHECK_EQ(seq.delete_at(2), 4);
    CHECK_EQ(seq.delete_at(0), 1);
    CHECK_EQ(seq.delete_at(0), 3);
    CHECK(seq.empty());
    CHECK_THROWS_AS(seq.delete_at(0), std::out_of_range);
}

TEST(2, "random inserts and deletes match a vector") {
    Rng rng;
    Seq seq;
    std::vector<int> model;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        if (model.empty() || rng.coin(0.55)) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size())));
            const int value = static_cast<int>(rng.uniform(0, 1'000'000));
            seq.insert_at(i, value);
            model.insert(model.begin() + static_cast<std::ptrdiff_t>(i), value);
        } else {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            REQUIRE_EQ(seq.delete_at(i), model[i]);
            model.erase(model.begin() + static_cast<std::ptrdiff_t>(i));
        }
        REQUIRE_EQ(seq.size(), model.size());
        if (step % 50 == 0) {
            REQUIRE_EQ(seq.to_vector(), model);
            check_shape(seq);
        }
    }
    CHECK_EQ(seq.to_vector(), model);
}

PERF(2, "half a million mixed edits in the middle", 3000) {
    Rng rng;
    std::vector<int> values(300'000);
    std::iota(values.begin(), values.end(), 0);
    Seq seq(values);
    for (int i = 0; i < 500'000; ++i) {
        const auto n = static_cast<std::int64_t>(seq.size());
        if (rng.coin()) {
            seq.insert_at(static_cast<std::size_t>(rng.uniform(0, n)), i);
        } else {
            seq.delete_at(static_cast<std::size_t>(rng.uniform(0, n - 1)));
        }
    }
    CHECK_GT(seq.size(), 0);
}

TEST(3, "fold over ranges with a sum augmentation") {
    using S = AvlSequence<int, SumAug>;
    S seq(std::vector<int>{1, 2, 3, 4, 5});
    CHECK_EQ(seq.fold(0, 5), 15);
    CHECK_EQ(seq.fold(1, 3), 5);
    CHECK_EQ(seq.fold(4, 5), 5);
    CHECK_EQ(seq.fold(2, 2), 0);
    CHECK_EQ(seq.fold(0, 0), 0);
    CHECK_EQ(seq.fold(5, 5), 0);
    CHECK_THROWS_AS(seq.fold(0, 6), std::out_of_range);
    CHECK_THROWS_AS(seq.fold(3, 2), std::out_of_range);
}

TEST(3, "fold on an empty sequence gives identity") {
    AvlSequence<int, SumAug> seq;
    CHECK_EQ(seq.fold(0, 0), 0);
    CHECK_THROWS_AS(seq.fold(0, 1), std::out_of_range);
}

TEST(3, "non-commutative folds keep left-to-right order") {
    using S = AvlSequence<std::string, ConcatAug>;
    Rng rng;
    std::vector<std::string> model;
    S seq;
    for (int i = 0; i < 300; ++i) {
        std::string letter(1, static_cast<char>('a' + rng.uniform(0, 25)));
        seq.insert_at(model.size(), letter);
        model.push_back(letter);
    }
    for (int step = 0; step < 5000; ++step) {
        INFO("step ", step);
        if (rng.coin(0.2)) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            seq.delete_at(i);
            model.erase(model.begin() + static_cast<std::ptrdiff_t>(i));
            seq.insert_at(i, "!");
            model.insert(model.begin() + static_cast<std::ptrdiff_t>(i), "!");
        }
        const auto l = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size())));
        const auto r = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(l), static_cast<std::int64_t>(model.size())));
        std::string expected;
        for (std::size_t i = l; i < r; ++i) {
            expected += model[i];
        }
        REQUIRE_EQ(seq.fold(l, r), expected);
    }
}

TEST(3, "folds follow in-place edits") {
    using S = AvlSequence<int, SumAug>;
    Rng rng;
    std::vector<int> model(500, 1);
    S seq(model);
    for (int step = 0; step < 5000; ++step) {
        INFO("step ", step);
        const auto i = static_cast<std::size_t>(rng.uniform(0, 499));
        const int value = static_cast<int>(rng.uniform(-100, 100));
        seq.set_at(i, value);
        model[i] = value;
        const auto l = static_cast<std::size_t>(rng.uniform(0, 500));
        const auto r = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(l), 500));
        REQUIRE_EQ(seq.fold(l, r), std::accumulate(model.begin() + static_cast<std::ptrdiff_t>(l),
                                                   model.begin() + static_cast<std::ptrdiff_t>(r), 0LL));
    }
}

TEST(3, "one million folds are not walks: combine count is logarithmic") {
    using S = AvlSequence<int, CountingAug>;
    Rng rng;
    const int n = 1 << 17;
    std::vector<int> values(static_cast<std::size_t>(n), 1);
    S seq(values);
    CountingAug::calls = 0;
    const int queries = 5000;
    for (int i = 0; i < queries; ++i) {
        const auto l = static_cast<std::size_t>(rng.uniform(0, n / 2));
        const auto r = static_cast<std::size_t>(rng.uniform(n / 2, n));
        REQUIRE_EQ(seq.fold(l, r), static_cast<long long>(r - l));
    }
    CHECK_LE(CountingAug::calls, static_cast<std::uint64_t>(queries * (8 * std::log2(n) + 16)));
}

PERF(3, "a million range folds on a large sequence", 4000) {
    using S = AvlSequence<int, SumAug>;
    std::vector<int> values(500'000);
    std::iota(values.begin(), values.end(), 0);
    S seq(values);
    Rng rng;
    long long sum = 0;
    for (int i = 0; i < 1'000'000; ++i) {
        const auto l = static_cast<std::size_t>(rng.uniform(0, 499'999));
        const auto r = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(l), 500'000));
        sum += seq.fold(l, r);
    }
    CHECK_GT(sum, 0);
}

TEST(4, "text edits update the content") {
    TextBuffer text("hello");
    CHECK_EQ(text.size(), 5);
    text.insert(5, '!');
    text.insert(0, '>');
    text.erase(3);
    CHECK_EQ(text.substr(0, text.size()), std::string(">helo!"));
    CHECK_EQ(text.at(1), 'h');
}

TEST(4, "substr handles every boundary") {
    const TextBuffer text("abcdef");
    CHECK_EQ(text.substr(0, 0), std::string());
    CHECK_EQ(text.substr(6, 6), std::string());
    CHECK_EQ(text.substr(2, 5), std::string("cde"));
    CHECK_THROWS_AS(text.substr(4, 3), std::out_of_range);
    CHECK_THROWS_AS(text.substr(0, 7), std::out_of_range);
    const TextBuffer empty;
    CHECK_EQ(empty.substr(0, 0), std::string());
    CHECK_EQ(empty.size(), 0);
}

TEST(4, "equal_ranges compares content, not position") {
    const TextBuffer text("abcabcxabc");
    CHECK(text.equal_ranges(0, 3, 3));
    CHECK(text.equal_ranges(0, 7, 3));
    CHECK(!text.equal_ranges(0, 1, 3));
    CHECK(!text.equal_ranges(3, 6, 2));
    CHECK(text.equal_ranges(2, 2, 4));  // same start
    CHECK(text.equal_ranges(5, 6, 0));  // zero length
    CHECK_THROWS_AS(text.equal_ranges(8, 0, 5), std::out_of_range);
}

TEST(4, "strings of different lengths with shared prefixes are not confused") {
    const TextBuffer text("aaaaaaaa");
    CHECK(text.equal_ranges(0, 3, 5));
    CHECK(!TextBuffer("aabaaa").equal_ranges(0, 3, 3));
    CHECK(TextBuffer("abab").equal_ranges(0, 2, 2));
}

TEST(4, "random edits and comparisons match a std::string") {
    Rng rng;
    std::string model = rng.text(200, "ab");
    TextBuffer text(model);
    for (int step = 0; step < 10'000; ++step) {
        INFO("step ", step);
        const auto action = rng.uniform(0, 3);
        if (action == 0) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size())));
            const char ch = rng.coin() ? 'a' : 'b';
            text.insert(i, ch);
            model.insert(i, 1, ch);
        } else if (action == 1 && !model.empty()) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
            text.erase(i);
            model.erase(i, 1);
        } else if (action == 2 && !model.empty()) {
            const auto l = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size())));
            const auto r = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(l), static_cast<std::int64_t>(model.size())));
            REQUIRE_EQ(text.substr(l, r), model.substr(l, r - l));
        } else if (!model.empty()) {
            const auto length = static_cast<std::size_t>(rng.uniform(0, std::min<std::int64_t>(30, static_cast<std::int64_t>(model.size()))));
            const auto a = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size() - length)));
            const auto b = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size() - length)));
            REQUIRE_EQ(text.equal_ranges(a, b, length), model.compare(a, length, model, b, length) == 0);
        }
        REQUIRE_EQ(text.size(), model.size());
    }
}

// Mostly-identical text defeats early-exit comparison, which is what a linear equal_ranges does.
PERF(4, "comparing long ranges does not walk them", 3000) {
    TextBuffer text(std::string(500'000, 'a'));
    Rng rng;
    std::uint64_t matches = 0;
    for (int i = 0; i < 300'000; ++i) {
        const auto a = static_cast<std::size_t>(rng.uniform(0, 100'000));
        const auto b = static_cast<std::size_t>(rng.uniform(100'001, 200'000));
        matches += text.equal_ranges(a, b, 300'000) ? 1 : 0;
        if (i % 100 == 0) {
            text.insert(static_cast<std::size_t>(rng.uniform(250'000, 300'000)), 'a');
        }
    }
    CHECK_EQ(matches, 300'000);
}
