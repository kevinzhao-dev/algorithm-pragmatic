#include <cmath>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Counts how many times keys are copied, moved or compared, which stands in for page writes.
class CountingKey {
public:
    explicit CountingKey(int value) : value_(value) {}
    CountingKey(const CountingKey& other) : value_(other.value_) { ++work(); }
    CountingKey(CountingKey&& other) noexcept : value_(other.value_) { ++work(); }
    CountingKey& operator=(const CountingKey& other) {
        value_ = other.value_;
        ++work();
        return *this;
    }
    CountingKey& operator=(CountingKey&& other) noexcept {
        value_ = other.value_;
        ++work();
        return *this;
    }
    friend bool operator<(const CountingKey& a, const CountingKey& b) {
        ++work();
        return a.value_ < b.value_;
    }
    int value() const { return value_; }

    static std::uint64_t& work() {
        static std::uint64_t count = 0;
        return count;
    }

private:
    int value_;
};

int key_value(int key) { return key; }
int key_value(const CountingKey& key) { return key.value(); }

template <typename Key>
struct Validator {
    const BTree<Key>& tree;
    std::vector<int> keys{};
    int leaf_depth = -1;

    void walk(const typename BTree<Key>::Node& node, int depth) {
        const std::size_t t = tree.min_degree();
        const bool is_root = &node == tree.root();
        CHECK_GE(node.keys.size(), is_root ? std::size_t{1} : t - 1);
        CHECK_LE(node.keys.size(), 2 * t - 1);
        if (node.is_leaf()) {
            if (leaf_depth == -1) {
                leaf_depth = depth;
            }
            CHECK_EQ(depth, leaf_depth);
            for (const Key& key : node.keys) {
                keys.push_back(key_value(key));
            }
            return;
        }
        REQUIRE_EQ(node.children.size(), node.keys.size() + 1);
        for (std::size_t i = 0; i < node.children.size(); ++i) {
            walk(*node.children[i], depth + 1);
            if (i < node.keys.size()) {
                keys.push_back(key_value(node.keys[i]));
            }
        }
    }
};

template <typename Key>
std::vector<int> check_tree(const BTree<Key>& tree, std::size_t expected_size) {
    CHECK_EQ(tree.size(), expected_size);
    if (tree.root() == nullptr) {
        CHECK_EQ(expected_size, 0);
        CHECK_EQ(tree.height(), -1);
        return {};
    }
    Validator<Key> validator{tree};
    validator.walk(*tree.root(), 0);
    CHECK_EQ(validator.keys.size(), expected_size);
    CHECK(std::ranges::is_sorted(validator.keys));
    CHECK_EQ(validator.leaf_depth, tree.height());
    return validator.keys;
}

// Builds a tree holding `count` consecutive keys starting at `first`, via random insertion.
template <typename Key = int>
BTree<Key> make_tree(std::size_t t, int first, int count, Rng& rng) {
    BTree<Key> tree(t);
    auto order = rng.permutation(static_cast<std::size_t>(count));
    for (int i : order) {
        tree.insert(Key(first + i));
    }
    return tree;
}

// Grows consecutive keys until the tree reaches exactly `height`.
template <typename Key = int>
BTree<Key> tree_of_height(std::size_t t, int first, int height, int* count) {
    BTree<Key> tree(t);
    int next = first;
    while (tree.height() < height) {
        tree.insert(Key(next++));
    }
    *count = next - first;
    return tree;
}

template <typename Key>
void check_emptied(const BTree<Key>& tree) {
    CHECK_EQ(tree.size(), 0);
    CHECK(tree.root() == nullptr);
    CHECK_EQ(tree.height(), -1);
}

// Combines around a fresh separator and confirms the result holds exactly the union, in order.
template <typename Key = int>
void combine_and_check(std::size_t t, int left_count, int right_count, Rng& rng) {
    auto left = make_tree<Key>(t, 0, left_count, rng);
    auto right = make_tree<Key>(t, left_count + 1, right_count, rng);
    BTree<Key> result = BTree<Key>::combine(std::move(left), Key(left_count), std::move(right));
    check_emptied(left);
    check_emptied(right);
    const auto keys = check_tree(result, static_cast<std::size_t>(left_count + right_count + 1));
    for (std::size_t i = 0; i < keys.size(); ++i) {
        REQUIRE_EQ(keys[i], static_cast<int>(i));
    }
}

}  // namespace

TEST(1, "an empty tree has height minus one") {
    const BTree<int> tree(2);
    CHECK_EQ(tree.height(), -1);
}

TEST(1, "a root-only tree has height zero") {
    BTree<int> tree(2);
    tree.insert(1);
    CHECK_EQ(tree.height(), 0);
}

TEST(1, "height grows exactly when the root splits") {
    BTree<int> tree(2);
    for (int key = 0; key < 3; ++key) {
        tree.insert(key);
    }
    CHECK_EQ(tree.height(), 0);
    tree.insert(3);
    CHECK_EQ(tree.height(), 1);
}

TEST(1, "height shrinks when the root collapses") {
    BTree<int> tree(2);
    for (int key = 0; key < 7; ++key) {
        tree.insert(key);
    }
    while (tree.height() > 0) {
        const int key = tree.root()->keys.front();
        REQUIRE(tree.erase(key));
        check_tree(tree, tree.size());
    }
    CHECK(tree.size() >= 1);
}

TEST(1, "erasing everything returns the height to minus one") {
    BTree<int> tree(2);
    tree.insert(1);
    tree.erase(1);
    CHECK_EQ(tree.height(), -1);
}

TEST(1, "height stays correct under random operations") {
    Rng rng;
    for (std::size_t t : {2u, 3u, 7u}) {
        INFO("t ", t);
        BTree<int> tree(t);
        std::set<int> model;
        for (int step = 0; step < 6000; ++step) {
            INFO("step ", step);
            const int key = static_cast<int>(rng.uniform(0, 500));
            if (rng.coin(0.55)) {
                REQUIRE_EQ(tree.insert(key), model.insert(key).second);
            } else {
                REQUIRE_EQ(tree.erase(key), model.erase(key) == 1);
            }
            if (step % 200 == 0) {
                check_tree(tree, model.size());
            }
        }
        check_tree(tree, model.size());
    }
}

TEST(2, "two empty trees combine into a single key") {
    BTree<int> left(3);
    BTree<int> right(3);
    BTree<int> result = BTree<int>::combine(std::move(left), 5, std::move(right));
    CHECK_EQ(check_tree(result, 1), std::vector<int>{5});
    check_emptied(left);
    check_emptied(right);
}

TEST(2, "different minimum degrees are rejected") {
    BTree<int> left(2);
    BTree<int> right(3);
    CHECK_THROWS_AS(BTree<int>::combine(std::move(left), 1, std::move(right)), std::invalid_argument);
}

TEST(2, "trees of equal height with room in the merged root") {
    Rng rng;
    combine_and_check(5, 3, 3, rng);
    combine_and_check(2, 1, 1, rng);
}

TEST(2, "trees of equal height whose merged root overflows") {
    Rng rng;
    BTree<int> left = make_tree(2, 0, 3, rng);
    BTree<int> right = make_tree(2, 4, 3, rng);
    REQUIRE_EQ(left.height(), right.height());
    BTree<int> result = BTree<int>::combine(std::move(left), 3, std::move(right));
    CHECK_EQ(result.height(), 1);
    CHECK_EQ(check_tree(result, 7), (std::vector<int>{0, 1, 2, 3, 4, 5, 6}));
}

TEST(2, "the result of an equal-height combine keeps working") {
    Rng rng;
    BTree<int> left = make_tree(3, 0, 30, rng);
    BTree<int> right = make_tree(3, 31, 30, rng);
    if (left.height() != right.height()) {
        return;
    }
    BTree<int> result = BTree<int>::combine(std::move(left), 30, std::move(right));
    for (int key = 100; key < 200; ++key) {
        CHECK(result.insert(key));
    }
    for (int key = 0; key < 30; key += 2) {
        CHECK(result.erase(key));
    }
    CHECK(result.contains(30));
    check_tree(result, 61 + 100 - 15);
}

TEST(2, "combining equal heights costs a constant in the tree size") {
    Rng rng;
    for (std::size_t t : {2u, 8u, 32u}) {
        INFO("t ", t);
        for (int target_height : {0, 1, 2}) {
            int left_count = 0;
            int right_count = 0;
            auto left = tree_of_height<CountingKey>(t, 0, target_height, &left_count);
            auto right = tree_of_height<CountingKey>(t, left_count + 1, target_height, &right_count);
            CountingKey separator(left_count);
            CountingKey::work() = 0;
            auto result = BTree<CountingKey>::combine(std::move(left), std::move(separator), std::move(right));
            CHECK_LE(CountingKey::work(), 12 * t);
            check_tree(result, static_cast<std::size_t>(left_count + right_count + 1));
        }
    }
}

TEST(3, "the left tree is one level taller") {
    Rng rng;
    int left_count = 0;
    auto left = tree_of_height<int>(3, 0, 2, &left_count);
    int right_count = 0;
    auto right = tree_of_height<int>(3, left_count + 1, 1, &right_count);
    REQUIRE_EQ(left.height(), right.height() + 1);
    BTree<int> result = BTree<int>::combine(std::move(left), left_count, std::move(right));
    const auto keys = check_tree(result, static_cast<std::size_t>(left_count + right_count + 1));
    for (std::size_t i = 0; i < keys.size(); ++i) {
        CHECK_EQ(keys[i], static_cast<int>(i));
    }
}

TEST(3, "the right tree is one level taller") {
    Rng rng;
    int left_count = 0;
    auto left = tree_of_height<int>(3, 0, 1, &left_count);
    int right_count = 0;
    auto right = tree_of_height<int>(3, left_count + 1, 2, &right_count);
    REQUIRE_EQ(right.height(), left.height() + 1);
    BTree<int> result = BTree<int>::combine(std::move(left), left_count, std::move(right));
    const auto keys = check_tree(result, static_cast<std::size_t>(left_count + right_count + 1));
    for (std::size_t i = 0; i < keys.size(); ++i) {
        CHECK_EQ(keys[i], static_cast<int>(i));
    }
}

TEST(3, "an overflow propagates into the shared root") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        INFO("trial ", trial);
        const auto t = static_cast<std::size_t>(rng.uniform(2, 3));
        int left_count = 0;
        auto left = tree_of_height<int>(t, 0, 2, &left_count);
        // Bulk up the taller tree so its spine is full and any merge must split upward.
        for (int key = left_count; key < left_count + 40; ++key) {
            left.insert(key);
        }
        left_count += 40;
        auto right = make_tree(t, left_count + 1, static_cast<int>(rng.uniform(1, 8)), rng);
        const auto right_size = right.size();
        if (left.height() != right.height() + 1) {
            continue;
        }
        BTree<int> result = BTree<int>::combine(std::move(left), left_count, std::move(right));
        check_tree(result, static_cast<std::size_t>(left_count) + right_size + 1);
    }
}

TEST(3, "one level of height difference costs a constant in the tree size") {
    for (std::size_t t : {2u, 8u, 32u}) {
        INFO("t ", t);
        for (bool left_taller : {true, false}) {
            int left_count = 0;
            int right_count = 0;
            auto left = tree_of_height<CountingKey>(t, 0, left_taller ? 2 : 1, &left_count);
            auto right = tree_of_height<CountingKey>(t, left_count + 1, left_taller ? 1 : 2, &right_count);
            CountingKey separator(left_count);
            CountingKey::work() = 0;
            auto result = BTree<CountingKey>::combine(std::move(left), std::move(separator), std::move(right));
            CHECK_LE(CountingKey::work(), 40 * t);
            check_tree(result, static_cast<std::size_t>(left_count + right_count + 1));
        }
    }
}

TEST(4, "an empty tree can be combined with a non-empty one on either side") {
    Rng rng;
    {
        BTree<int> left(2);
        BTree<int> right = make_tree(2, 10, 20, rng);
        BTree<int> result = BTree<int>::combine(std::move(left), 5, std::move(right));
        const auto keys = check_tree(result, 21);
        CHECK_EQ(keys.front(), 5);
    }
    {
        BTree<int> left = make_tree(2, 0, 20, rng);
        BTree<int> right(2);
        BTree<int> result = BTree<int>::combine(std::move(left), 50, std::move(right));
        const auto keys = check_tree(result, 21);
        CHECK_EQ(keys.back(), 50);
    }
}

TEST(4, "random sizes on both sides are combined correctly") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        INFO("trial ", trial);
        const auto t = static_cast<std::size_t>(rng.uniform(2, 6));
        const int left_count = static_cast<int>(rng.uniform(0, rng.coin() ? 8 : 400));
        const int right_count = static_cast<int>(rng.uniform(0, rng.coin() ? 8 : 400));
        combine_and_check(t, left_count, right_count, rng);
    }
}

TEST(4, "a very short tree grafts onto a very tall one on both sides") {
    Rng rng;
    for (bool left_taller : {true, false}) {
        INFO("left taller ", left_taller);
        int left_count = 0;
        int right_count = 0;
        auto left = tree_of_height<int>(2, 0, left_taller ? 9 : 0, &left_count);
        auto right = tree_of_height<int>(2, left_count + 1, left_taller ? 0 : 9, &right_count);
        BTree<int> result = BTree<int>::combine(std::move(left), left_count, std::move(right));
        const auto keys = check_tree(result, static_cast<std::size_t>(left_count + right_count + 1));
        for (std::size_t i = 0; i < keys.size(); ++i) {
            REQUIRE_EQ(keys[i], static_cast<int>(i));
        }
    }
}

TEST(4, "repeated combines keep an index valid") {
    Rng rng;
    BTree<int> accumulated = make_tree(3, 0, 100, rng);
    int next = 100;
    for (int round = 0; round < 200; ++round) {
        const int separator = next++;
        const int count = static_cast<int>(rng.uniform(0, 30));
        BTree<int> piece = make_tree(3, next, count, rng);
        next += count;
        accumulated = BTree<int>::combine(std::move(accumulated), separator, std::move(piece));
    }
    const auto keys = check_tree(accumulated, static_cast<std::size_t>(next));
    for (std::size_t i = 0; i < keys.size(); ++i) {
        REQUIRE_EQ(keys[i], static_cast<int>(i));
    }
}

TEST(4, "the cost scales with the height difference, not the tree size") {
    const std::pair<std::size_t, int> shapes[] = {{2, 6}, {2, 12}, {8, 2}, {8, 4}};
    for (const auto& [t, taller] : shapes) {
        for (int shorter : {0, 1}) {
            for (bool left_taller : {true, false}) {
                INFO("t ", t, " taller ", taller, " shorter ", shorter, " left taller ", left_taller);
                int left_count = 0;
                int right_count = 0;
                auto left = tree_of_height<CountingKey>(t, 0, left_taller ? taller : shorter, &left_count);
                auto right = tree_of_height<CountingKey>(t, left_count + 1, left_taller ? shorter : taller, &right_count);
                const std::uint64_t gap = static_cast<std::uint64_t>(std::abs(left.height() - right.height()));
                CountingKey separator(left_count);
                CountingKey::work() = 0;
                auto result = BTree<CountingKey>::combine(std::move(left), std::move(separator), std::move(right));
                CHECK_LE(CountingKey::work(), 16 * t * (gap + 1));
                check_tree(result, static_cast<std::size_t>(left_count + right_count + 1));
            }
        }
    }
}

TEST(5, "concatenating with an empty tree returns the other one") {
    Rng rng;
    BTree<int> left = make_tree(3, 0, 10, rng);
    BTree<int> empty(3);
    BTree<int> result = BTree<int>::concat(std::move(left), std::move(empty));
    CHECK_EQ(check_tree(result, 10).size(), 10);
    BTree<int> empty2(3);
    BTree<int> result2 = BTree<int>::concat(std::move(empty2), std::move(result));
    CHECK_EQ(check_tree(result2, 10).size(), 10);
}

TEST(5, "two empty trees concatenate to an empty tree") {
    BTree<int> left(3);
    BTree<int> right(3);
    BTree<int> result = BTree<int>::concat(std::move(left), std::move(right));
    check_emptied(result);
}

TEST(5, "keys from both sides appear in order") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        INFO("trial ", trial);
        const auto t = static_cast<std::size_t>(rng.uniform(2, 5));
        const int left_count = static_cast<int>(rng.uniform(0, 300));
        const int right_count = static_cast<int>(rng.uniform(0, 300));
        BTree<int> left = make_tree(t, 0, left_count, rng);
        BTree<int> right = make_tree(t, left_count, right_count, rng);
        BTree<int> result = BTree<int>::concat(std::move(left), std::move(right));
        const auto keys = check_tree(result, static_cast<std::size_t>(left_count + right_count));
        for (std::size_t i = 0; i < keys.size(); ++i) {
            REQUIRE_EQ(keys[i], static_cast<int>(i));
        }
    }
}

TEST(5, "concatenating two partitions rewrites only logarithmically many keys") {
    Rng rng;
    const std::size_t t = 8;
    auto left = make_tree<CountingKey>(t, 0, 50'000, rng);
    auto right = make_tree<CountingKey>(t, 50'000, 50'000, rng);
    const std::uint64_t levels = static_cast<std::uint64_t>(left.height() + right.height() + 2);
    CountingKey::work() = 0;
    auto result = BTree<CountingKey>::concat(std::move(left), std::move(right));
    CHECK_LE(CountingKey::work(), 40 * t * levels);
    check_tree(result, 100'000);
}
