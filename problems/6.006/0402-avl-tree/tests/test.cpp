#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;

using Tree = AvlTree<int>;
using Node = Tree::Node;

static_assert(std::forward_iterator<Tree::iterator>);

namespace {

// ---- field checks shared by every stage -------------------------------------------------

template <typename T, typename A>
void collect_inorder(const typename AvlTree<T, A>::Node* node, std::vector<const typename AvlTree<T, A>::Node*>& out) {
    if (node == nullptr) {
        return;
    }
    collect_inorder<T, A>(node->left, out);
    out.push_back(node);
    collect_inorder<T, A>(node->right, out);
}

// Verifies parent links and stored height/size against a recomputation; returns (height, size).
template <typename N>
std::pair<int, std::size_t> check_fields(const N* node) {
    if (node == nullptr) {
        return {-1, 0};
    }
    if (node->left != nullptr) {
        REQUIRE(node->left->parent == node);
    }
    if (node->right != nullptr) {
        REQUIRE(node->right->parent == node);
    }
    const auto [lh, ls] = check_fields(node->left);
    const auto [rh, rs] = check_fields(node->right);
    const int h = 1 + std::max(lh, rh);
    const std::size_t s = 1 + ls + rs;
    REQUIRE_EQ(node->height, h);
    REQUIRE_EQ(node->size, s);
    return {h, s};
}

// Same, and additionally requires |skew| <= 1 everywhere.
template <typename N>
std::pair<int, std::size_t> check_balanced(const N* node) {
    if (node == nullptr) {
        return {-1, 0};
    }
    const auto [lh, ls] = check_balanced(node->left);
    const auto [rh, rs] = check_balanced(node->right);
    REQUIRE(std::abs(lh - rh) <= 1);
    return {1 + std::max(lh, rh), 1 + ls + rs};
}

template <typename Tr>
void check_tree(const Tr& tree) {
    if (tree.root() == nullptr) {
        CHECK_EQ(tree.size(), 0);
        return;
    }
    CHECK(tree.root()->parent == nullptr);
    check_fields(tree.root());
    CHECK_EQ(tree.root()->size, tree.size());
}

template <typename Tr>
void check_avl(const Tr& tree) {
    check_tree(tree);
    check_balanced(tree.root());
}

template <typename T, typename A>
std::vector<int> values_of(const AvlTree<T, A>& tree) {
    std::vector<int> out;
    for (const auto& item : tree) {
        if constexpr (std::is_same_v<T, Tracked>) {
            out.push_back(item.value());
        } else {
            out.push_back(item);
        }
    }
    return out;
}

// ---- augmentations used by the stage 4 tests ----------------------------------------------

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

template <typename A, typename T, typename N>
typename A::Value recompute(const N* node) {
    if (node == nullptr) {
        return A::identity();
    }
    return A::combine(A::combine(recompute<A, T, N>(node->left), A::of(node->item)),
                      recompute<A, T, N>(node->right));
}

template <typename A, typename T, typename N>
void check_aug(const N* node) {
    if (node == nullptr) {
        return;
    }
    check_aug<A, T, N>(node->left);
    check_aug<A, T, N>(node->right);
    const auto expected = A::combine(
        A::combine(node->left != nullptr ? node->left->aug : A::identity(), A::of(node->item)),
        node->right != nullptr ? node->right->aug : A::identity());
    REQUIRE_EQ(node->aug, expected);
}

template <typename T, typename A>
std::vector<int> ints_of(const AvlTree<T, A>& tree) {
    std::vector<int> out;
    for (const auto& item : tree) {
        out.push_back(item);
    }
    return out;
}

}  // namespace

// ------------------------------------------------------------------------------------------

TEST(1, "a new tree is empty") {
    Tree tree;
    CHECK(tree.empty());
    CHECK_EQ(tree.size(), 0);
    CHECK(tree.root() == nullptr);
    CHECK(tree.begin() == tree.end());
}

TEST(1, "insertions keep traversal order and correct fields") {
    Tree tree;
    Node* a = tree.insert_after(nullptr, 5);
    CHECK_EQ(a->height, 0);
    CHECK_EQ(a->size, 1);
    Node* b = tree.insert_before(a, 3);
    Node* c = tree.insert_after(a, 8);
    tree.insert_before(c, 7);
    tree.insert_after(b, 4);
    CHECK_EQ(values_of(tree), std::vector<int>{3, 4, 5, 7, 8});
    CHECK_EQ(tree.size(), 5);
    check_tree(tree);
}

TEST(1, "a single element is both first and last") {
    Tree tree;
    Node* a = tree.insert_before(nullptr, 1);
    CHECK(tree.root() == a);
    CHECK(Tree::subtree_first(a) == a);
    CHECK(Tree::subtree_last(a) == a);
    CHECK(Tree::successor(a) == nullptr);
    CHECK(Tree::predecessor(a) == nullptr);
    CHECK_EQ(tree.erase(a), 1);
    CHECK(tree.empty());
    CHECK(tree.root() == nullptr);
}

TEST(1, "erasing leaves, one-child nodes and two-child nodes") {
    Tree tree;
    Node* m = tree.insert_after(nullptr, 5);
    Node* l = tree.insert_before(m, 2);
    Node* r = tree.insert_after(m, 8);
    Node* rl = tree.insert_before(r, 7);
    Node* rr = tree.insert_after(r, 9);
    CHECK_EQ(tree.erase(m), 5);
    CHECK_EQ(values_of(tree), std::vector<int>{2, 7, 8, 9});
    CHECK_EQ(l->item, 2);
    CHECK_EQ(r->item, 8);
    CHECK_EQ(rl->item, 7);
    CHECK_EQ(rr->item, 9);
    check_tree(tree);
    CHECK_EQ(tree.erase(l), 2);
    CHECK_EQ(tree.erase(rr), 9);
    CHECK_EQ(values_of(tree), std::vector<int>{7, 8});
    check_tree(tree);
}

TEST(1, "random operations match a model and keep every handle") {
    Rng rng;
    Tree tree;
    std::list<int> model;
    struct Known {
        Node* node;
        std::list<int>::iterator it;
    };
    std::vector<Known> known;
    int fresh = 0;
    for (int step = 0; step < 4000; ++step) {
        INFO("step ", step);
        if (known.empty() || rng.coin(0.6)) {
            const int value = fresh++;
            std::size_t anchor_index = 0;
            const int mode = known.empty() ? 2 : static_cast<int>(rng.uniform(0, 3));
            Node* node = nullptr;
            std::list<int>::iterator it;
            if (mode < 2) {
                anchor_index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(known.size()) - 1));
                Node* anchor = known[anchor_index].node;
                auto anchor_it = known[anchor_index].it;
                if (mode == 0) {
                    node = tree.insert_before(anchor, value);
                    it = model.insert(anchor_it, value);
                } else {
                    node = tree.insert_after(anchor, value);
                    it = model.insert(std::next(anchor_it), value);
                }
            } else if (mode == 2) {
                node = tree.insert_before(nullptr, value);
                it = model.insert(model.end(), value);
            } else {
                node = tree.insert_after(nullptr, value);
                it = model.insert(model.begin(), value);
            }
            known.push_back({node, it});
        } else {
            const auto index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(known.size()) - 1));
            const auto [node, it] = known[index];
            REQUIRE_EQ(tree.erase(node), *it);
            model.erase(it);
            known[index] = known.back();
            known.pop_back();
        }
        REQUIRE_EQ(tree.size(), model.size());
        if (step % 16 == 0) {
            REQUIRE_EQ(values_of(tree), std::vector<int>(model.begin(), model.end()));
            check_tree(tree);
            for (const auto& entry : known) {
                REQUIRE_EQ(entry.node->item, *entry.it);
            }
        }
    }
    CHECK_EQ(values_of(tree), std::vector<int>(model.begin(), model.end()));
}

TEST(1, "every element is released") {
    const auto before = Tracked::live();
    {
        AvlTree<Tracked> tree;
        std::vector<AvlTree<Tracked>::Node*> nodes;
        for (int i = 0; i < 400; ++i) {
            nodes.push_back(tree.insert_before(nullptr, Tracked(i)));
        }
        for (std::size_t i = 0; i < nodes.size(); i += 2) {
            CHECK_EQ(tree.erase(nodes[i]).value(), static_cast<int>(i));
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(2, "rotating left then right restores the exact tree") {
    Rng rng;
    Tree tree;
    std::vector<Node*> nodes;
    for (int i = 0; i < 60; ++i) {
        if (nodes.empty() || rng.coin(0.3)) {
            nodes.push_back(tree.insert_before(nullptr, i));
        } else {
            nodes.push_back(tree.insert_after(nodes[static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(nodes.size()) - 1))], i));
        }
    }
    check_tree(tree);
    const auto order = values_of(tree);

    struct Links {
        Node* parent;
        Node* left;
        Node* right;
    };
    const auto snapshot = [&] {
        std::vector<Links> links;
        for (Node* node : nodes) {
            links.push_back({node->parent, node->left, node->right});
        }
        return links;
    };

    for (Node* node : nodes) {
        if (node->right == nullptr) {
            continue;
        }
        INFO("node ", node->item);
        const auto before = snapshot();
        Node* y = node->right;
        Node* top = tree.rotate_left(node);
        REQUIRE(top == y);
        CHECK(y->left == node);
        CHECK(node->parent == y);
        CHECK_EQ(values_of(tree), order);
        CHECK_EQ(y->height, check_fields(y).first);
        CHECK_EQ(y->size, check_fields(y).second);

        Node* back = tree.rotate_right(y);
        REQUIRE(back == node);
        CHECK_EQ(values_of(tree), order);
        const auto after = snapshot();
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            REQUIRE(after[i].parent == before[i].parent);
            REQUIRE(after[i].left == before[i].left);
            REQUIRE(after[i].right == before[i].right);
        }
    }
    check_tree(tree);
}

TEST(2, "rotating the root updates the root pointer") {
    Tree tree;
    Node* root = tree.insert_after(nullptr, 1);
    Node* child = tree.insert_after(root, 2);  // with two nodes there is nothing to rebalance
    Node* top = tree.rotate_left(root);
    CHECK(top == child);
    CHECK(tree.root() == child);
    CHECK(child->left == root);
    CHECK(root->right == nullptr);
    CHECK(root->parent == child);
    CHECK_EQ(values_of(tree), std::vector<int>{1, 2});
}

TEST(3, "the tree stays balanced under random operations") {
    Rng rng;
    Tree tree;
    std::vector<Node*> nodes;
    int fresh = 0;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        if (nodes.empty() || rng.coin(0.55)) {
            Node* anchor = nodes.empty() ? nullptr : nodes[static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(nodes.size()) - 1))];
            Node* node = rng.coin() ? tree.insert_before(anchor, fresh) : tree.insert_after(anchor, fresh);
            nodes.push_back(node);
            ++fresh;
        } else {
            const auto index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(nodes.size()) - 1));
            tree.erase(nodes[index]);
            nodes[index] = nodes.back();
            nodes.pop_back();
        }
        if (step % 20 == 0) {
            check_avl(tree);
        }
    }
    check_avl(tree);
}

TEST(3, "sorted appends and sorted prepends stay logarithmic") {
    for (int direction = 0; direction < 2; ++direction) {
        Tree tree;
        for (int i = 0; i < 20'000; ++i) {
            if (direction == 0) {
                tree.insert_before(nullptr, i);
            } else {
                tree.insert_after(nullptr, i);
            }
        }
        check_avl(tree);
        const double limit = 1.45 * std::log2(static_cast<double>(tree.size()) + 2);
        CHECK_LE(tree.root()->height, limit);
    }
}

TEST(3, "erasing the root over and over stays balanced") {
    Tree tree;
    for (int i = 0; i < 5000; ++i) {
        tree.insert_before(nullptr, i);
    }
    while (tree.root() != nullptr) {
        tree.erase(tree.root());
        if (tree.size() % 37 == 0) {
            check_avl(tree);
        }
    }
    CHECK(tree.empty());
}

TEST(3, "erasing in sorted order forces rotations on the way up") {
    Tree tree;
    std::vector<Node*> nodes;
    for (int i = 0; i < 4000; ++i) {
        nodes.push_back(tree.insert_before(nullptr, i));
    }
    for (Node* node : nodes) {
        tree.erase(node);
        check_avl(tree);
    }
}

TEST(3, "handles survive rebalancing") {
    Rng rng;
    AvlTree<int> tree;
    std::vector<AvlTree<int>::Node*> nodes;
    std::vector<int> values;
    for (int i = 0; i < 2000; ++i) {
        nodes.push_back(tree.insert_before(nullptr, i * 3));
        values.push_back(i * 3);
    }
    for (int step = 0; step < 1000; ++step) {
        const auto index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(nodes.size()) - 1));
        tree.erase(nodes[index]);
        nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(index));
        values.erase(values.begin() + static_cast<std::ptrdiff_t>(index));
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        CHECK_EQ(nodes[i]->item, values[i]);
    }
}

PERF(3, "a million sorted appends and erases", 3000) {
    Tree tree;
    std::vector<Node*> nodes;
    nodes.reserve(1'000'000);
    for (int i = 0; i < 1'000'000; ++i) {
        nodes.push_back(tree.insert_before(nullptr, i));
    }
    REQUIRE(tree.root()->height <= 30);
    for (int i = 0; i < 1'000'000; ++i) {
        tree.erase(nodes[static_cast<std::size_t>(i)]);
    }
    CHECK(tree.empty());
}

TEST(4, "the sum augmentation stays correct through mutations") {
    Rng rng;
    using SumTree = AvlTree<int, SumAug>;
    SumTree tree;
    std::vector<SumTree::Node*> nodes;
    for (int step = 0; step < 6000; ++step) {
        INFO("step ", step);
        if (nodes.empty() || rng.coin(0.6)) {
            const int value = static_cast<int>(rng.uniform(-1000, 1000));
            nodes.push_back(tree.insert_before(nullptr, value));
        } else {
            const auto index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(nodes.size()) - 1));
            tree.erase(nodes[index]);
            nodes[index] = nodes.back();
            nodes.pop_back();
        }
        if (step % 31 == 0 && tree.root() != nullptr) {
            check_aug<SumAug, int>(tree.root());
            check_avl(tree);
            CHECK_EQ(tree.root()->aug, recompute<SumAug, int>(tree.root()));
        }
    }
}

TEST(4, "the augmentation respects order: concatenation is not commutative") {
    Rng rng;
    using TextTree = AvlTree<std::string, ConcatAug>;
    TextTree tree;
    std::vector<TextTree::Node*> nodes;
    std::vector<std::string> model;
    for (int step = 0; step < 3000; ++step) {
        INFO("step ", step);
        const auto pos = model.empty() ? std::size_t{0}
                                       : static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(model.size()) - 1));
        if (model.empty() || rng.coin(0.65)) {
            std::string text(1, static_cast<char>('a' + rng.uniform(0, 25)));
            TextTree::Node* node = tree.insert_before(model.empty() ? nullptr : nodes[pos], text);
            nodes.insert(nodes.begin() + static_cast<std::ptrdiff_t>(model.empty() ? 0 : pos), node);
            model.insert(model.begin() + static_cast<std::ptrdiff_t>(model.empty() ? 0 : pos), text);
        } else {
            tree.erase(nodes[pos]);
            nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(pos));
            model.erase(model.begin() + static_cast<std::ptrdiff_t>(pos));
        }
        if (step % 17 == 0) {
            std::string expected;
            for (const auto& piece : model) {
                expected += piece;
            }
            REQUIRE_EQ(tree.root() != nullptr ? tree.root()->aug : std::string(), expected);
        }
    }
}

TEST(4, "refresh recomputes after an in-place change") {
    Rng rng;
    using SumTree = AvlTree<int, SumAug>;
    SumTree tree;
    std::vector<SumTree::Node*> nodes;
    long long total = 0;
    for (int i = 0; i < 1000; ++i) {
        const int value = static_cast<int>(rng.uniform(0, 100));
        nodes.push_back(tree.insert_before(nullptr, value));
        total += value;
    }
    CHECK_EQ(tree.root()->aug, total);
    for (int step = 0; step < 2000; ++step) {
        auto* node = nodes[static_cast<std::size_t>(rng.uniform(0, 999))];
        const int value = static_cast<int>(rng.uniform(-500, 500));
        total += value - node->item;
        node->item = value;
        tree.refresh(node);
        REQUIRE_EQ(tree.root()->aug, total);
    }
    check_aug<SumAug, int>(tree.root());
}

TEST(4, "updates call combine only O(log n) times") {
    using CountTree = AvlTree<int, CountingAug>;
    Rng rng;
    CountTree tree;
    std::vector<CountTree::Node*> nodes;
    const int n = 1 << 16;
    for (int i = 0; i < n; ++i) {
        nodes.push_back(tree.insert_before(nullptr, i));
    }
    const auto lg = std::log2(static_cast<double>(n));

    CountingAug::calls = 0;
    const int rounds = 20'000;
    for (int i = 0; i < rounds; ++i) {
        auto* node = nodes[static_cast<std::size_t>(rng.uniform(0, n - 1))];
        tree.refresh(node);
    }
    CHECK_LE(CountingAug::calls, static_cast<std::uint64_t>(rounds * (6 * lg + 8)));

    CountingAug::calls = 0;
    for (int i = 0; i < rounds; ++i) {
        const auto index = static_cast<std::size_t>(rng.uniform(0, n - 1));
        tree.erase(nodes[index]);
        nodes[index] = tree.insert_before(nullptr, i);
    }
    CHECK_LE(CountingAug::calls, static_cast<std::uint64_t>(rounds * (24 * lg + 24)));
}

TEST(5, "build gives a balanced tree with correct fields") {
    for (std::size_t n : {0u, 1u, 2u, 3u, 7u, 8u, 100u, 1024u, 100000u}) {
        INFO("n=", n);
        std::vector<int> values(n);
        for (std::size_t i = 0; i < n; ++i) {
            values[i] = static_cast<int>(i);
        }
        const Tree tree = Tree::build(values);
        CHECK_EQ(values_of(tree), values);
        CHECK_EQ(tree.size(), n);
        check_avl(tree);
    }
}

TEST(5, "build maintains the augmentation with O(n) combines") {
    using CountTree = AvlTree<int, CountingAug>;
    std::vector<int> values(100'000);
    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = 1;
    }
    CountingAug::calls = 0;
    const auto tree = CountTree::build(values);
    CHECK_LE(CountingAug::calls, static_cast<std::uint64_t>(3 * values.size()));
    CHECK_EQ(tree.root()->aug, static_cast<long long>(values.size()));
    check_aug<CountingAug, int>(tree.root());
}

TEST(5, "build works with move-only items") {
    AvlTree<std::unique_ptr<int>> tree;
    std::vector<std::unique_ptr<int>> items;
    for (int i = 0; i < 50; ++i) {
        items.push_back(std::make_unique<int>(i));
    }
    tree = AvlTree<std::unique_ptr<int>>::build(std::move(items));
    int expected = 0;
    for (const auto& item : tree) {
        CHECK_EQ(*item, expected);
        ++expected;
    }
    CHECK_EQ(expected, 50);
}

TEST(5, "move construction and assignment transfer ownership") {
    const auto before = Tracked::live();
    {
        AvlTree<Tracked> a = AvlTree<Tracked>::build({Tracked(1), Tracked(2), Tracked(3)});
        AvlTree<Tracked> b(std::move(a));
        CHECK(a.empty());  // NOLINT: moved-from state is part of the contract
        CHECK_EQ(b.size(), 3);

        AvlTree<Tracked> c;
        c.insert_before(nullptr, Tracked(9));
        c = std::move(b);
        CHECK(b.empty());  // NOLINT
        CHECK_EQ(values_of(c), std::vector<int>{1, 2, 3});
        c = std::move(c);  // NOLINT: self-move must be harmless
        CHECK_EQ(c.size(), 3);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(5, "a built tree keeps working with inserts and erases") {
    Rng rng;
    std::vector<int> values(2000);
    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = static_cast<int>(i);
    }
    Tree tree = Tree::build(values);
    for (int step = 0; step < 2000; ++step) {
        Node* node = tree.insert_before(nullptr, 100000 + step);
        if (step % 2 == 0) {
            tree.erase(node);
        }
    }
    check_avl(tree);
    CHECK_EQ(tree.size(), 2000 + 1000);
}

PERF(5, "two million items built in linear time", 3000) {
    std::vector<int> values(2'000'000);
    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = static_cast<int>(i);
    }
    const Tree tree = Tree::build(values);
    CHECK_LE(tree.root()->height, 25);
    long long sum = 0;
    for (int value : tree) {
        sum += value;
    }
    CHECK_EQ(sum, 2'000'000LL * 1'999'999 / 2);
}
