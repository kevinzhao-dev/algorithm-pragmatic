#include <algorithm>
#include <cstdint>
#include <iterator>
#include <list>
#include <memory>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;

using Tree = BinaryTree<int>;
using Node = Tree::Node;

static_assert(std::forward_iterator<Tree::iterator>);

namespace {

// Six stack-allocated nodes wired by hand, so stage 1 needs no insertion code at all.
//         a
//       /   \
//      b     c
//     / \
//    d   e
//   /
//  f
struct HandTree {
    Node a{1}, b{2}, c{3}, d{4}, e{5}, f{6};
    HandTree() {
        link(a, &b, &c);
        link(b, &d, &e);
        link(d, &f, nullptr);
    }
    static void link(Node& parent, Node* left, Node* right) {
        parent.left = left;
        parent.right = right;
        if (left != nullptr) {
            left->parent = &parent;
        }
        if (right != nullptr) {
            right->parent = &parent;
        }
    }
};

template <typename T>
std::vector<int> values_of(const BinaryTree<T>& tree) {
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

// Confirms parent pointers agree with child pointers everywhere and that sizes match.
template <typename T>
void check_links(const BinaryTree<T>& tree) {
    using N = typename BinaryTree<T>::Node;
    if (tree.root() == nullptr) {
        CHECK_EQ(tree.size(), 0);
        return;
    }
    CHECK(tree.root()->parent == nullptr);
    std::size_t visited = 0;
    std::vector<N*> stack{tree.root()};
    while (!stack.empty()) {
        N* node = stack.back();
        stack.pop_back();
        ++visited;
        if (node->left != nullptr) {
            REQUIRE(node->left->parent == node);
            stack.push_back(node->left);
        }
        if (node->right != nullptr) {
            REQUIRE(node->right->parent == node);
            stack.push_back(node->right);
        }
    }
    CHECK_EQ(visited, tree.size());
}

int subtree_height(const Node* node) {
    return node == nullptr ? -1 : 1 + std::max(subtree_height(node->left), subtree_height(node->right));
}

}  // namespace

TEST(1, "subtree_first and subtree_last find the extremes of a subtree") {
    HandTree t;
    CHECK(Tree::subtree_first(&t.a) == &t.f);
    CHECK(Tree::subtree_last(&t.a) == &t.c);
    CHECK(Tree::subtree_first(&t.b) == &t.f);
    CHECK(Tree::subtree_last(&t.b) == &t.e);
    CHECK(Tree::subtree_first(&t.c) == &t.c);
    CHECK(Tree::subtree_last(&t.f) == &t.f);
}

TEST(1, "null subtrees stay null") {
    CHECK(Tree::subtree_first(nullptr) == nullptr);
    CHECK(Tree::subtree_last(nullptr) == nullptr);
}

TEST(1, "successor walks the traversal order") {
    HandTree t;
    CHECK(Tree::successor(&t.f) == &t.d);
    CHECK(Tree::successor(&t.d) == &t.b);
    CHECK(Tree::successor(&t.b) == &t.e);
    CHECK(Tree::successor(&t.e) == &t.a);
    CHECK(Tree::successor(&t.a) == &t.c);
    CHECK(Tree::successor(&t.c) == nullptr);
}

TEST(1, "predecessor walks the traversal order backwards") {
    HandTree t;
    CHECK(Tree::predecessor(&t.c) == &t.a);
    CHECK(Tree::predecessor(&t.a) == &t.e);
    CHECK(Tree::predecessor(&t.e) == &t.b);
    CHECK(Tree::predecessor(&t.b) == &t.d);
    CHECK(Tree::predecessor(&t.d) == &t.f);
    CHECK(Tree::predecessor(&t.f) == nullptr);
}

TEST(1, "a single node has no neighbours") {
    Node lone{7};
    CHECK(Tree::subtree_first(&lone) == &lone);
    CHECK(Tree::subtree_last(&lone) == &lone);
    CHECK(Tree::successor(&lone) == nullptr);
    CHECK(Tree::predecessor(&lone) == nullptr);
}

TEST(2, "a new tree is empty") {
    Tree tree;
    CHECK(tree.empty());
    CHECK_EQ(tree.size(), 0);
    CHECK(tree.root() == nullptr);
    CHECK(tree.begin() == tree.end());
}

TEST(2, "the first insertion becomes the root either way") {
    Tree before;
    Node* a = before.insert_before(nullptr, 1);
    CHECK(before.root() == a);
    CHECK_EQ(before.size(), 1);

    Tree after;
    Node* b = after.insert_after(nullptr, 2);
    CHECK(after.root() == b);
}

TEST(2, "insert_before and insert_after obey the attachment rule") {
    Tree tree;
    Node* m = tree.insert_after(nullptr, 5);
    Node* l = tree.insert_before(m, 3);
    Node* r = tree.insert_after(m, 8);
    CHECK(m->left == l);
    CHECK(m->right == r);

    // m already has a left child, so the node hangs off l, the last node of m's left subtree.
    Node* x = tree.insert_before(m, 4);
    CHECK(l->right == x);
    CHECK(x->parent == l);

    // Symmetric for insert_after.
    Node* y = tree.insert_after(m, 7);
    CHECK(r->left == y);
    CHECK(y->parent == r);

    CHECK_EQ(values_of(tree), std::vector<int>{3, 4, 5, 7, 8});
    check_links(tree);
}

TEST(2, "null anchors mean the two ends of the sequence") {
    Tree tree;
    tree.insert_before(nullptr, 2);
    tree.insert_before(nullptr, 3);
    tree.insert_after(nullptr, 1);
    CHECK_EQ(values_of(tree), std::vector<int>{1, 2, 3});
    CHECK_EQ(tree.size(), 3);
    check_links(tree);
}

TEST(2, "iterators work with standard algorithms") {
    Tree tree;
    for (int value : {4, 2, 7}) {
        tree.insert_before(nullptr, value);
    }
    std::vector<int> copied(tree.begin(), tree.end());
    CHECK_EQ(copied, std::vector<int>{4, 2, 7});
    CHECK_EQ(std::distance(tree.begin(), tree.end()), 3);
    auto it = tree.begin();
    *it = 40;
    CHECK_EQ(*tree.begin(), 40);
    auto previous = it++;
    CHECK_EQ(*previous, 40);
    CHECK_EQ(*it, 2);
}

TEST(2, "random insertions match a model list") {
    Rng rng;
    Tree tree;
    std::list<int> model;
    std::vector<std::pair<Node*, std::list<int>::iterator>> known;
    for (int step = 0; step < 500; ++step) {
        INFO("step ", step);
        const int value = static_cast<int>(rng.uniform(0, 1'000'000));
        if (known.empty() || rng.coin(0.1)) {
            if (rng.coin()) {
                known.emplace_back(tree.insert_before(nullptr, value), model.insert(model.end(), value));
            } else {
                known.emplace_back(tree.insert_after(nullptr, value), model.insert(model.begin(), value));
            }
        } else {
            auto& [node, it] = known[static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(known.size()) - 1))];
            if (rng.coin()) {
                known.emplace_back(tree.insert_before(node, value), model.insert(it, value));
            } else {
                known.emplace_back(tree.insert_after(node, value), model.insert(std::next(it), value));
            }
        }
        REQUIRE_EQ(tree.size(), model.size());
        REQUIRE_EQ(values_of(tree), std::vector<int>(model.begin(), model.end()));
        check_links(tree);
    }
}

TEST(2, "every node is released") {
    const auto before = Tracked::live();
    {
        BinaryTree<Tracked> tree;
        for (int i = 0; i < 1000; ++i) {
            tree.insert_before(nullptr, Tracked(i));
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(2, "destroying a degenerate tree does not overflow the stack") {
    Tree tree;
    Node* tail = nullptr;
    for (int i = 0; i < 1'000'000; ++i) {
        tail = tree.insert_after(tail, i);
    }
    CHECK_EQ(tree.size(), 1'000'000);
}

TEST(3, "erasing a leaf") {
    Tree tree;
    Node* m = tree.insert_after(nullptr, 2);
    Node* l = tree.insert_before(m, 1);
    Node* r = tree.insert_after(m, 3);
    CHECK_EQ(tree.erase(l), 1);
    CHECK(m->left == nullptr);
    CHECK_EQ(tree.erase(r), 3);
    CHECK(m->right == nullptr);
    CHECK_EQ(tree.size(), 1);
    check_links(tree);
}

TEST(3, "erasing a node with one child") {
    Tree tree;
    Node* a = tree.insert_after(nullptr, 2);
    Node* b = tree.insert_after(a, 4);
    Node* c = tree.insert_before(b, 3);
    CHECK_EQ(tree.erase(b), 4);
    CHECK(a->right == c);
    CHECK(c->parent == a);
    CHECK_EQ(values_of(tree), std::vector<int>{2, 3});
    check_links(tree);
}

TEST(3, "erasing a node with two children keeps its neighbours' pointers intact") {
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
    CHECK_EQ(tree.size(), 4);
    check_links(tree);
}

TEST(3, "successor being the right child, and erasing the root repeatedly") {
    Tree tree;
    Node* a = tree.insert_after(nullptr, 2);
    tree.insert_before(a, 1);
    Node* c = tree.insert_after(a, 3);
    tree.insert_after(c, 4);
    tree.erase(a);  // successor of a is its right child c
    CHECK_EQ(values_of(tree), std::vector<int>{1, 3, 4});
    check_links(tree);
    while (tree.root() != nullptr) {
        tree.erase(tree.root());
        check_links(tree);
    }
    CHECK(tree.empty());
    CHECK_EQ(tree.size(), 0);
}

TEST(3, "erase is exception-free for values and releases what it removes") {
    const auto before = Tracked::live();
    {
        BinaryTree<Tracked> tree;
        std::vector<BinaryTree<Tracked>::Node*> nodes;
        for (int i = 0; i < 300; ++i) {
            nodes.push_back(tree.insert_before(nullptr, Tracked(i)));
        }
        for (std::size_t i = 0; i < nodes.size(); i += 3) {
            CHECK_EQ(tree.erase(nodes[i]).value(), static_cast<int>(i));
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(3, "random erases keep order and every other handle") {
    Rng rng;
    Tree tree;
    std::vector<Node*> nodes;
    std::vector<int> model;
    for (int i = 0; i < 3000; ++i) {
        nodes.push_back(tree.insert_before(nullptr, i));
        model.push_back(i);
    }
    while (!nodes.empty()) {
        INFO("remaining ", nodes.size());
        const auto index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(nodes.size()) - 1));
        Node* victim = nodes[index];
        const int value = model[index];
        REQUIRE_EQ(tree.erase(victim), value);
        nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(index));
        model.erase(model.begin() + static_cast<std::ptrdiff_t>(index));
        REQUIRE_EQ(values_of(tree), model);
        if (nodes.size() % 7 == 0) {
            check_links(tree);
            for (std::size_t i = 0; i < nodes.size(); ++i) {
                REQUIRE_EQ(nodes[i]->item, model[i]);
            }
        }
    }
    CHECK(tree.empty());
}

TEST(4, "build keeps the order and balances the tree") {
    for (std::size_t n : {1u, 2u, 3u, 7u, 8u, 1000u, 1023u, 1024u, 100000u}) {
        INFO("n=", n);
        std::vector<int> values(n);
        for (std::size_t i = 0; i < n; ++i) {
            values[i] = static_cast<int>(i);
        }
        const Tree tree = Tree::build(values);
        CHECK_EQ(values_of(tree), values);
        CHECK_EQ(tree.size(), n);
        check_links(tree);
        int log2n = 0;
        while ((std::size_t{1} << (log2n + 1)) <= n) {
            ++log2n;
        }
        CHECK_LE(subtree_height(tree.root()), log2n);
    }
}

TEST(4, "build of an empty sequence gives an empty tree") {
    const Tree tree = Tree::build({});
    CHECK(tree.empty());
    CHECK(tree.root() == nullptr);
}

TEST(4, "build moves items in instead of copying them") {
    BinaryTree<std::unique_ptr<int>> tree;
    std::vector<std::unique_ptr<int>> items;
    for (int i = 0; i < 100; ++i) {
        items.push_back(std::make_unique<int>(i));
    }
    tree = BinaryTree<std::unique_ptr<int>>::build(std::move(items));
    int expected = 0;
    for (const auto& item : tree) {
        CHECK_EQ(*item, expected);
        ++expected;
    }
    CHECK_EQ(expected, 100);
}

TEST(4, "move construction and assignment transfer ownership") {
    const auto before = Tracked::live();
    {
        BinaryTree<Tracked> a = BinaryTree<Tracked>::build({Tracked(1), Tracked(2), Tracked(3)});
        BinaryTree<Tracked> b(std::move(a));
        CHECK(a.empty());  // NOLINT: moved-from state is part of the contract
        CHECK_EQ(b.size(), 3);

        BinaryTree<Tracked> c;
        c.insert_before(nullptr, Tracked(9));
        c = std::move(b);
        CHECK(b.empty());  // NOLINT
        CHECK_EQ(values_of(c), std::vector<int>{1, 2, 3});
        c = std::move(c);  // NOLINT: self-move must be harmless
        CHECK_EQ(c.size(), 3);
    }
    CHECK_EQ(Tracked::live(), before);
}

PERF(4, "three million items built and walked", 2500) {
    std::vector<int> values(3'000'000);
    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = static_cast<int>(i);
    }
    const Tree tree = Tree::build(values);
    long long sum = 0;
    for (int value : tree) {
        sum += value;
    }
    CHECK_EQ(sum, 3'000'000LL * 2'999'999 / 2);
}

PERF(4, "iterating a chain of a million nodes stays linear", 1500) {
    Tree tree;
    Node* tail = nullptr;
    for (int i = 0; i < 1'000'000; ++i) {
        tail = tree.insert_after(tail, i);
    }
    long long sum = 0;
    for (int round = 0; round < 3; ++round) {
        for (int value : tree) {
            sum += value;
        }
    }
    CHECK_GT(sum, 0);
}

TEST(5, "a new set is empty") {
    BstSet<int> set;
    CHECK(set.empty());
    CHECK_EQ(set.size(), 0);
    CHECK_EQ(set.height(), -1);
    CHECK_EQ(set.find_min(), std::optional<int>());
    CHECK_EQ(set.find_max(), std::optional<int>());
    CHECK_EQ(set.find_next(1), std::optional<int>());
    CHECK(!set.erase(1));
}

TEST(5, "insert, contains and sorted order") {
    BstSet<int> set;
    for (int key : {5, 2, 8, 1, 9, 3}) {
        CHECK(set.insert(key));
    }
    CHECK(!set.insert(5));
    CHECK_EQ(set.size(), 6);
    CHECK_EQ(set.to_vector(), std::vector<int>{1, 2, 3, 5, 8, 9});
    CHECK(set.contains(3));
    CHECK(!set.contains(4));
    CHECK_EQ(set.find_min(), std::optional<int>(1));
    CHECK_EQ(set.find_max(), std::optional<int>(9));
}

TEST(5, "find_next and find_prev work for present and absent keys") {
    BstSet<int> set;
    for (int key : {10, 20, 30, 40}) {
        set.insert(key);
    }
    CHECK_EQ(set.find_next(10), std::optional<int>(20));
    CHECK_EQ(set.find_next(15), std::optional<int>(20));
    CHECK_EQ(set.find_next(5), std::optional<int>(10));
    CHECK_EQ(set.find_next(40), std::optional<int>());
    CHECK_EQ(set.find_next(100), std::optional<int>());
    CHECK_EQ(set.find_prev(10), std::optional<int>());
    CHECK_EQ(set.find_prev(25), std::optional<int>(20));
    CHECK_EQ(set.find_prev(40), std::optional<int>(30));
    CHECK_EQ(set.find_prev(1000), std::optional<int>(40));
}

TEST(5, "erase keeps the ordering") {
    BstSet<int> set;
    for (int key : {5, 2, 8, 1, 3, 7, 9, 6}) {
        set.insert(key);
    }
    CHECK(set.erase(5));   // two children
    CHECK(set.erase(1));   // leaf
    CHECK(!set.erase(100));
    CHECK_EQ(set.to_vector(), std::vector<int>{2, 3, 6, 7, 8, 9});
    CHECK_EQ(set.size(), 6);
}

TEST(5, "a custom comparator flips the order") {
    BstSet<int, std::greater<int>> set{std::greater<int>{}};
    for (int key : {1, 3, 2}) {
        set.insert(key);
    }
    CHECK_EQ(set.to_vector(), std::vector<int>{3, 2, 1});
    CHECK_EQ(set.find_min(), std::optional<int>(3));
}

TEST(5, "sorted insertion degenerates into a chain") {
    BstSet<int> set;
    for (int key = 0; key < 2000; ++key) {
        set.insert(key);
    }
    CHECK_EQ(set.height(), 1999);
}

TEST(5, "random insertion stays reasonably shallow") {
    Rng rng;
    BstSet<int> set;
    for (int i = 0; i < 20'000; ++i) {
        set.insert(static_cast<int>(rng.uniform(0, 1'000'000'000)));
    }
    // Random BSTs have height about 2.99 lg n; 60 leaves plenty of room.
    CHECK_LE(set.height(), 60);
}

TEST(5, "random operations match std::set") {
    Rng rng;
    BstSet<int> set;
    std::set<int> model;
    for (int step = 0; step < 100'000; ++step) {
        INFO("step ", step);
        const int key = static_cast<int>(rng.uniform(0, 500));
        const auto op = rng.uniform(0, 4);
        if (op == 0) {
            REQUIRE_EQ(set.insert(key), model.insert(key).second);
        } else if (op == 1) {
            REQUIRE_EQ(set.erase(key), model.erase(key) > 0);
        } else if (op == 2) {
            REQUIRE_EQ(set.contains(key), model.contains(key));
        } else if (op == 3) {
            const auto it = model.upper_bound(key);
            REQUIRE_EQ(set.find_next(key), it == model.end() ? std::optional<int>() : std::optional<int>(*it));
        } else {
            const auto it = model.lower_bound(key);
            REQUIRE_EQ(set.find_prev(key), it == model.begin() ? std::optional<int>() : std::optional<int>(*std::prev(it)));
        }
        REQUIRE_EQ(set.size(), model.size());
    }
    CHECK_EQ(set.to_vector(), std::vector<int>(model.begin(), model.end()));
}

PERF(5, "half a million random keys inserted, searched and erased", 2500) {
    Rng rng;
    BstSet<int> set;
    std::vector<int> keys(500'000);
    for (int& key : keys) {
        key = static_cast<int>(rng.uniform(0, 2'000'000'000));
    }
    for (int key : keys) {
        set.insert(key);
    }
    for (int key : keys) {
        REQUIRE(set.contains(key));
    }
    for (int key : keys) {
        set.erase(key);
    }
    CHECK(set.empty());
}
