#include <cmath>
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

// Walks the tree and enforces every structural invariant, returning the keys in order.
template <typename Key>
struct Validator {
    const BTree<Key>& tree;
    std::vector<Key> keys{};
    int leaf_depth = -1;

    void walk(const typename BTree<Key>::Node& node, int depth) {
        const std::size_t t = tree.min_degree();
        const bool is_root = &node == tree.root();
        CHECK_GE(node.keys.size(), is_root ? std::size_t{1} : t - 1);
        CHECK_LE(node.keys.size(), 2 * t - 1);
        CHECK(std::ranges::is_sorted(node.keys));
        if (node.is_leaf()) {
            if (leaf_depth == -1) {
                leaf_depth = depth;
            }
            CHECK_EQ(depth, leaf_depth);
            keys.insert(keys.end(), node.keys.begin(), node.keys.end());
            return;
        }
        REQUIRE_EQ(node.children.size(), node.keys.size() + 1);
        for (std::size_t i = 0; i < node.children.size(); ++i) {
            walk(*node.children[i], depth + 1);
            if (i < node.keys.size()) {
                keys.push_back(node.keys[i]);
            }
        }
    }
};

// A freshly inserted tree must be balanced with height at most log_t((n + 1) / 2).
template <typename Key>
int check_tree(const BTree<Key>& tree, std::size_t expected_size) {
    CHECK_EQ(tree.size(), expected_size);
    CHECK_EQ(tree.empty(), expected_size == 0);
    if (tree.root() == nullptr) {
        CHECK_EQ(expected_size, 0);
        return -1;
    }
    Validator<Key> validator{tree};
    validator.walk(*tree.root(), 0);
    CHECK_EQ(validator.keys.size(), expected_size);
    CHECK(std::ranges::is_sorted(validator.keys));
    const double t = static_cast<double>(tree.min_degree());
    CHECK_LE(validator.leaf_depth,
             static_cast<int>(std::floor(std::log((static_cast<double>(expected_size) + 1.0) / 2.0) / std::log(t) + 1e-9)));
    return validator.leaf_depth;
}

std::vector<int> keys_of(const BTree<int>& tree) {
    Validator<int> validator{tree};
    if (tree.root() != nullptr) {
        validator.walk(*tree.root(), 0);
    }
    return validator.keys;
}

}  // namespace

TEST(1, "a minimum degree below two is rejected") {
    CHECK_THROWS_AS(BTree<int>(1), std::invalid_argument);
    CHECK_THROWS_AS(BTree<int>(0), std::invalid_argument);
}

TEST(1, "a new tree is empty") {
    const BTree<int> tree(3);
    CHECK_EQ(tree.min_degree(), 3);
    CHECK_EQ(tree.size(), 0);
    CHECK(tree.empty());
    CHECK(tree.root() == nullptr);
    CHECK(!tree.contains(1));
}

TEST(1, "inserted keys can be found") {
    BTree<int> tree(2);
    for (int key : {5, 1, 9, 3, 7}) {
        CHECK(tree.insert(key));
    }
    for (int key : {1, 3, 5, 7, 9}) {
        CHECK(tree.contains(key));
    }
    for (int key : {0, 2, 4, 6, 8, 10}) {
        CHECK(!tree.contains(key));
    }
    check_tree(tree, 5);
}

TEST(1, "inserting a duplicate is refused and changes nothing") {
    BTree<int> tree(2);
    CHECK(tree.insert(4));
    CHECK(!tree.insert(4));
    CHECK_EQ(tree.size(), 1);
    check_tree(tree, 1);
}

TEST(1, "minimum degree two splits full nodes correctly") {
    BTree<int> tree(2);
    for (int key = 0; key < 200; ++key) {
        tree.insert(key);
        check_tree(tree, static_cast<std::size_t>(key) + 1);
    }
    CHECK_EQ(keys_of(tree).size(), 200);
}

TEST(1, "descending inserts stay balanced") {
    BTree<int> tree(3);
    for (int key = 1000; key > 0; --key) {
        tree.insert(key);
    }
    check_tree(tree, 1000);
}

TEST(1, "move construction transfers the tree and leaves the source reusable") {
    BTree<int> source(2);
    source.insert(1);
    source.insert(2);
    BTree<int> moved(std::move(source));
    CHECK_EQ(moved.size(), 2);
    CHECK(moved.contains(2));
    CHECK_EQ(source.size(), 0);
    CHECK(source.root() == nullptr);
    CHECK(source.insert(7));
    CHECK(source.contains(7));
}

TEST(1, "move assignment replaces the old contents") {
    BTree<int> a(2);
    BTree<int> b(5);
    a.insert(1);
    b.insert(2);
    a = std::move(b);
    CHECK_EQ(a.min_degree(), 5);
    CHECK(a.contains(2));
    CHECK(!a.contains(1));
}

TEST(1, "no keys leak after destruction") {
    const auto before = Tracked::live();
    {
        BTree<TrackedKey> tree(3);
        for (int key = 0; key < 500; ++key) {
            tree.insert(TrackedKey(key));
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(1, "random inserts match std::set") {
    Rng rng;
    for (std::size_t t : {2u, 3u, 5u, 16u}) {
        INFO("min degree ", t);
        BTree<int> tree(t);
        std::set<int> model;
        for (int step = 0; step < 4000; ++step) {
            INFO("step ", step);
            const int key = static_cast<int>(rng.uniform(0, 2000));
            REQUIRE_EQ(tree.insert(key), model.insert(key).second);
            REQUIRE(tree.contains(key));
            const int probe = static_cast<int>(rng.uniform(0, 2000));
            REQUIRE_EQ(tree.contains(probe), model.contains(probe));
            if (step % 500 == 0) {
                check_tree(tree, model.size());
            }
        }
        check_tree(tree, model.size());
        CHECK_EQ(keys_of(tree), std::vector<int>(model.begin(), model.end()));
    }
}

TEST(1, "a lookup touches one node per level") {
    Rng rng;
    BTree<Counted<int>> tree(8);
    for (int key : rng.permutation(50'000)) {
        tree.insert(Counted<int>(key));
    }
    const int height = check_tree(tree, 50'000);
    Counted<int>::comparisons() = 0;
    int found = 0;
    for (int i = 0; i < 5000; ++i) {
        found += tree.contains(Counted<int>(i * 10));
    }
    CHECK_EQ(found, 5000);
    CHECK_LE(Counted<int>::comparisons(),
             static_cast<std::uint64_t>(5000 * 2 * 2 * 8 * (height + 1)));
}

PERF(1, "one million inserts and lookups", 4000) {
    Rng rng;
    BTree<int> tree(32);
    const auto keys = rng.permutation(1'000'000);
    for (int key : keys) {
        tree.insert(key);
    }
    int found = 0;
    for (int key : keys) {
        found += tree.contains(key);
    }
    CHECK_EQ(found, 1'000'000);
}

TEST(2, "erasing from an empty tree reports absence") {
    BTree<int> tree(2);
    CHECK(!tree.erase(1));
    CHECK_EQ(tree.size(), 0);
}

TEST(2, "erasing a missing key changes nothing") {
    BTree<int> tree(2);
    tree.insert(1);
    CHECK(!tree.erase(2));
    CHECK_EQ(tree.size(), 1);
    CHECK(tree.contains(1));
}

TEST(2, "erasing the last key empties the tree") {
    BTree<int> tree(2);
    tree.insert(1);
    CHECK(tree.erase(1));
    CHECK(tree.empty());
    CHECK(tree.root() == nullptr);
}

TEST(2, "an emptied tree can be refilled") {
    BTree<int> tree(2);
    tree.insert(1);
    tree.erase(1);
    tree.insert(2);
    CHECK(tree.contains(2));
    check_tree(tree, 1);
}

TEST(2, "erasing keys inside internal nodes keeps the tree valid") {
    BTree<int> tree(2);
    for (int key = 0; key < 64; ++key) {
        tree.insert(key);
    }
    for (int key : {31, 15, 47, 7, 23, 39, 55}) {
        CHECK(tree.erase(key));
    }
    check_tree(tree, 57);
    CHECK(!tree.contains(31));
}

TEST(2, "the root shrinks when its children merge") {
    BTree<int> tree(2);
    for (int key = 1; key <= 7; ++key) {
        tree.insert(key);
    }
    for (int key : {1, 2, 3, 4, 5, 6}) {
        CHECK(tree.erase(key));
        check_tree(tree, 7 - static_cast<std::size_t>(key));
    }
    CHECK(tree.contains(7));
}

TEST(2, "erasing in order drains the tree") {
    BTree<int> tree(3);
    for (int key = 0; key < 500; ++key) {
        tree.insert(key);
    }
    for (int key = 0; key < 500; ++key) {
        CHECK(tree.erase(key));
        check_tree(tree, 499 - static_cast<std::size_t>(key));
    }
    CHECK(tree.root() == nullptr);
}

TEST(2, "no keys leak through deletions") {
    const auto before = Tracked::live();
    {
        BTree<TrackedKey> tree(2);
        for (int key = 0; key < 400; ++key) {
            tree.insert(TrackedKey(key));
        }
        for (int key = 0; key < 400; key += 2) {
            tree.erase(TrackedKey(key));
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(2, "random operations match std::set") {
    Rng rng;
    for (std::size_t t : {2u, 3u, 4u, 9u}) {
        INFO("min degree ", t);
        BTree<int> tree(t);
        std::set<int> model;
        for (int step = 0; step < 12'000; ++step) {
            INFO("step ", step);
            const int key = static_cast<int>(rng.uniform(0, 600));
            if (rng.coin(0.55)) {
                REQUIRE_EQ(tree.insert(key), model.insert(key).second);
            } else {
                REQUIRE_EQ(tree.erase(key), model.erase(key) == 1);
            }
            if (step % 300 == 0) {
                check_tree(tree, model.size());
            }
        }
        check_tree(tree, model.size());
        CHECK_EQ(keys_of(tree), std::vector<int>(model.begin(), model.end()));
    }
}

PERF(2, "mixed inserts and erases", 4000) {
    Rng rng;
    BTree<int> tree(16);
    const auto keys = rng.permutation(1'000'000);
    for (int key : keys) {
        tree.insert(key);
    }
    for (std::size_t i = 0; i < keys.size(); i += 2) {
        CHECK(tree.erase(keys[i]));
    }
    CHECK_EQ(tree.size(), 500'000);
}

TEST(3, "lower_bound returns the first key at or after the target") {
    BTree<int> tree(2);
    for (int key : {10, 20, 30, 40, 50}) {
        tree.insert(key);
    }
    CHECK_EQ(tree.lower_bound(5), std::optional<int>(10));
    CHECK_EQ(tree.lower_bound(30), std::optional<int>(30));
    CHECK_EQ(tree.lower_bound(31), std::optional<int>(40));
    CHECK_EQ(tree.lower_bound(50), std::optional<int>(50));
    CHECK(!tree.lower_bound(51).has_value());
}

TEST(3, "lower_bound on an empty tree finds nothing") {
    const BTree<int> tree(4);
    CHECK(!tree.lower_bound(1).has_value());
}

TEST(3, "scan visits keys in order and excludes the upper bound") {
    BTree<int> tree(2);
    for (int key = 0; key < 100; ++key) {
        tree.insert(key * 2);
    }
    std::vector<int> seen;
    tree.scan(11, 21, [&](const int& key) { seen.push_back(key); });
    CHECK_EQ(seen, (std::vector<int>{12, 14, 16, 18, 20}));
}

TEST(3, "an empty range visits nothing") {
    BTree<int> tree(2);
    tree.insert(1);
    int count = 0;
    tree.scan(5, 5, [&](const int&) { ++count; });
    tree.scan(2, 1, [&](const int&) { ++count; });
    CHECK_EQ(count, 0);
}

TEST(3, "random range queries match std::set") {
    Rng rng;
    BTree<int> tree(4);
    std::set<int> model;
    for (int step = 0; step < 5000; ++step) {
        const int key = static_cast<int>(rng.uniform(0, 3000));
        if (rng.coin(0.7)) {
            tree.insert(key);
            model.insert(key);
        } else {
            tree.erase(key);
            model.erase(key);
        }
        if (step % 25 == 0) {
            INFO("step ", step);
            const int low = static_cast<int>(rng.uniform(0, 3000));
            const int high = static_cast<int>(rng.uniform(low, 3000));
            std::vector<int> seen;
            tree.scan(low, high, [&](const int& k) { seen.push_back(k); });
            std::vector<int> expected(model.lower_bound(low), model.lower_bound(high));
            REQUIRE_EQ(seen, expected);
            const auto it = model.lower_bound(low);
            const auto found = tree.lower_bound(low);
            if (it == model.end()) {
                REQUIRE(!found.has_value());
            } else {
                REQUIRE_EQ(found.value(), *it);
            }
        }
    }
}

TEST(3, "a short scan touches only a root-to-leaf path and the range") {
    Rng rng;
    const std::size_t t = 8;
    BTree<Counted<int>> tree(t);
    for (int key : rng.permutation(200'000)) {
        tree.insert(Counted<int>(key));
    }
    const int height = check_tree(tree, 200'000);
    Counted<int>::comparisons() = 0;
    int seen = 0;
    tree.scan(Counted<int>(50'000), Counted<int>(50'100), [&](const Counted<int>&) { ++seen; });
    CHECK_EQ(seen, 100);
    CHECK_LE(Counted<int>::comparisons(),
             static_cast<std::uint64_t>(6 * 2 * static_cast<int>(t) * (height + 2) + 4 * seen));
}
