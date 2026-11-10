#include <algorithm>
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"
#include "tree_text.hpp"

using practice::Rng;

namespace {

// A concrete tree used throughout: 70 has skew 2 (no left child, right subtree of height 1).
//            50
//          /    \
//        30      70
//       /  \       \
//      20   40      90
//     /            /
//    10           80
constexpr std::string_view kSample = "((((. 10 .) 20 .) 30 (. 40 .)) 50 (. 70 ((. 80 .) 90 .)))";

// Independent oracle for the deletion contract, written over a simple pointer tree.
struct OracleNode {
    int key;
    OracleNode* left = nullptr;
    OracleNode* right = nullptr;
};

OracleNode* build_oracle(const TreeNode* node, std::vector<std::unique_ptr<OracleNode>>& pool) {
    if (node == nullptr) {
        return nullptr;
    }
    pool.push_back(std::make_unique<OracleNode>(OracleNode{node->key}));
    OracleNode* out = pool.back().get();
    out->left = build_oracle(node->left.get(), pool);
    out->right = build_oracle(node->right.get(), pool);
    return out;
}

std::string oracle_text(const OracleNode* node) {
    if (node == nullptr) {
        return ".";
    }
    return "(" + oracle_text(node->left) + " " + std::to_string(node->key) + " " + oracle_text(node->right) + ")";
}

// Applies the swap-down contract to find the expected final shape.
void oracle_erase(OracleNode*& root, int key) {
    OracleNode** slot = &root;
    while (*slot != nullptr && (*slot)->key != key) {
        slot = key < (*slot)->key ? &(*slot)->left : &(*slot)->right;
    }
    if (*slot == nullptr) {
        return;
    }
    while ((*slot)->left != nullptr || (*slot)->right != nullptr) {
        OracleNode* node = *slot;
        OracleNode** next;
        if (node->left != nullptr) {
            next = &node->left;
            while ((*next)->right != nullptr) {
                next = &(*next)->right;
            }
        } else {
            next = &node->right;
            while ((*next)->left != nullptr) {
                next = &(*next)->left;
            }
        }
        std::swap(node->key, (*next)->key);
        slot = next;
    }
    *slot = nullptr;
}

void oracle_insert(OracleNode*& root, int key, std::vector<std::unique_ptr<OracleNode>>& pool) {
    OracleNode** slot = &root;
    while (*slot != nullptr) {
        if ((*slot)->key == key) {
            return;
        }
        slot = key < (*slot)->key ? &(*slot)->left : &(*slot)->right;
    }
    pool.push_back(std::make_unique<OracleNode>(OracleNode{key}));
    *slot = pool.back().get();
}

// Applies a rotation already covered by the exact-shape tests above, then checks balance directly.
bool oracle_balanced_after(const TreeNode* root, int key, bool left_rotation) {
    Tree copy = clone_tree(root);
    if (left_rotation) {
        rotate_left_at(copy, key);
    } else {
        rotate_right_at(copy, key);
    }
    std::function<int(const TreeNode*, bool&)> height = [&](const TreeNode* node, bool& ok) {
        if (node == nullptr) {
            return -1;
        }
        const int l = height(node->left.get(), ok);
        const int r = height(node->right.get(), ok);
        ok = ok && std::abs(r - l) <= 1;
        return 1 + std::max(l, r);
    };
    bool ok = true;
    height(copy.get(), ok);
    return ok;
}

}  // namespace

TEST(1, "heights of small trees") {
    CHECK_EQ(subtree_height(nullptr), -1);
    const Tree leaf = parse_tree("(. 5 .)");
    CHECK_EQ(subtree_height(leaf.get()), 0);
    const Tree sample = parse_tree(kSample);
    CHECK_EQ(subtree_height(sample.get()), 3);
    CHECK_EQ(subtree_height(sample->right.get()), 2);
}

TEST(1, "skew compares the right subtree to the left") {
    const Tree sample = parse_tree(kSample);
    CHECK_EQ(skew(sample.get()), 0);
    CHECK_EQ(skew(sample->left.get()), -1);       // 30: left height 1, right height 0
    CHECK_EQ(skew(sample->right.get()), 2);       // 70: no left child, right subtree height 1
    const Tree leaf = parse_tree("(. 5 .)");
    CHECK_EQ(skew(leaf.get()), 0);
}

TEST(1, "unbalanced nodes are listed in key order with their skew") {
    const Tree sample = parse_tree(kSample);
    CHECK_EQ(unbalanced_nodes(sample.get()), std::vector<std::pair<int, int>>{{70, 2}});
}

TEST(1, "a balanced tree reports nothing") {
    const Tree tree = parse_tree("((. 1 .) 2 (. 3 .))");
    CHECK(unbalanced_nodes(tree.get()).empty());
    CHECK(unbalanced_nodes(nullptr).empty());
}

TEST(1, "skews of magnitude above 2 are reported too") {
    const Tree chain = parse_tree("(. 1 (. 2 (. 3 (. 4 .))))");
    CHECK_EQ(unbalanced_nodes(chain.get()), std::vector<std::pair<int, int>>{{1, 3}, {2, 2}});
    const Tree left_heavy = parse_tree("((((. 1 .) 2 .) 3 .) 4 .)");
    CHECK_EQ(unbalanced_nodes(left_heavy.get()), std::vector<std::pair<int, int>>{{3, -2}, {4, -3}});
}

TEST(2, "insert adds a leaf in BST position") {
    Tree tree = parse_tree("((. 3 .) 8 .)");
    bst_insert(tree, 5);
    CHECK_EQ(to_text(tree.get()), std::string("((. 3 (. 5 .)) 8 .)"));
    bst_insert(tree, 1);
    CHECK_EQ(to_text(tree.get()), std::string("(((. 1 .) 3 (. 5 .)) 8 .)"));
    bst_insert(tree, 9);
    CHECK_EQ(to_text(tree.get()), std::string("(((. 1 .) 3 (. 5 .)) 8 (. 9 .))"));
}

TEST(2, "inserting into an empty tree creates the root") {
    Tree tree;
    bst_insert(tree, 4);
    CHECK_EQ(to_text(tree.get()), std::string("(. 4 .)"));
}

TEST(2, "inserting an existing key changes nothing") {
    Tree tree = parse_tree("((. 3 .) 8 .)");
    bst_insert(tree, 3);
    CHECK_EQ(to_text(tree.get()), std::string("((. 3 .) 8 .)"));
}

TEST(2, "erase removes a leaf directly") {
    Tree tree = parse_tree("((. 3 .) 8 (. 9 .))");
    bst_erase(tree, 3);
    CHECK_EQ(to_text(tree.get()), std::string("(. 8 (. 9 .))"));
}

TEST(2, "erase of a node with a left subtree swaps down through the predecessor") {
    Tree tree = parse_tree("((. 3 (. 5 .)) 8 (. 9 .))");
    bst_erase(tree, 8);
    CHECK_EQ(to_text(tree.get()), std::string("((. 3 .) 5 (. 9 .))"));
}

TEST(2, "erase with only a right subtree swaps down through the successor") {
    Tree tree = parse_tree("(. 2 ((. 4 .) 5 (. 7 .)))");
    bst_erase(tree, 2);
    CHECK_EQ(to_text(tree.get()), std::string("(. 4 (. 5 (. 7 .)))"));
}

TEST(2, "erase keeps swapping until the key reaches a leaf") {
    // 8's predecessor is 5, which has a left child 4: the walk continues below it.
    Tree tree = parse_tree("(((. 2 .) 3 ((. 4 .) 5 .)) 8 (. 9 .))");
    bst_erase(tree, 8);
    CHECK_EQ(to_text(tree.get()), std::string("(((. 2 .) 3 (. 4 .)) 5 (. 9 .))"));
}

TEST(2, "erasing the only node and a missing key") {
    Tree tree = parse_tree("(. 4 .)");
    bst_erase(tree, 7);
    CHECK_EQ(to_text(tree.get()), std::string("(. 4 .)"));
    bst_erase(tree, 4);
    CHECK(tree == nullptr);
}

TEST(2, "random insert and erase sequences match the contract exactly") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        INFO("round ", round);
        Tree tree;
        std::vector<std::unique_ptr<OracleNode>> pool;
        OracleNode* oracle = nullptr;
        for (int step = 0; step < 80; ++step) {
            INFO("step ", step);
            const int key = static_cast<int>(rng.uniform(1, 40));
            if (rng.coin(0.6)) {
                bst_insert(tree, key);
                oracle_insert(oracle, key, pool);
            } else {
                bst_erase(tree, key);
                oracle_erase(oracle, key);
            }
            REQUIRE_EQ(to_text(tree.get()), oracle_text(oracle));
        }
    }
}

TEST(3, "rotate left at the root") {
    Tree tree = parse_tree("((. 1 .) 2 ((. 3 .) 4 (. 5 .)))");
    rotate_left_at(tree, 2);
    CHECK_EQ(to_text(tree.get()), std::string("(((. 1 .) 2 (. 3 .)) 4 (. 5 .))"));
}

TEST(3, "rotate right below the root keeps the rest in place") {
    Tree tree = parse_tree("(((. 1 .) 3 (. 4 .)) 5 (. 9 .))");
    rotate_right_at(tree, 3);
    CHECK_EQ(to_text(tree.get()), std::string("((. 1 (. 3 (. 4 .))) 5 (. 9 .))"));
}

TEST(3, "rotations in both directions undo each other") {
    Rng rng;
    for (int round = 0; round < 100; ++round) {
        Tree tree;
        const auto n = static_cast<int>(rng.uniform(2, 30));
        for (int i = 0; i < n; ++i) {
            bst_insert(tree, static_cast<int>(rng.uniform(1, 200)));
        }
        const std::string before = to_text(tree.get());
        const int key = tree->key;
        if (tree->right != nullptr) {
            const int right_key = tree->right->key;
            rotate_left_at(tree, key);
            CHECK_EQ(tree->key, right_key);
            rotate_right_at(tree, right_key);
            CHECK_EQ(to_text(tree.get()), before);
        }
    }
}

TEST(3, "illegal rotations throw") {
    Tree tree = parse_tree("(. 1 .)");
    CHECK_THROWS_AS(rotate_left_at(tree, 1), std::invalid_argument);
    CHECK_THROWS_AS(rotate_right_at(tree, 1), std::invalid_argument);
    CHECK_THROWS_AS(rotate_left_at(tree, 99), std::invalid_argument);
    CHECK_THROWS_AS(rotate_right_at(tree, 99), std::invalid_argument);
    CHECK_EQ(to_text(tree.get()), std::string("(. 1 .)"));
}

TEST(3, "is_height_balanced checks every node, not just the root") {
    CHECK(is_height_balanced(nullptr));
    const Tree ok = parse_tree("((. 1 .) 2 (. 3 .))");
    CHECK(is_height_balanced(ok.get()));
    const Tree bad_inside = parse_tree("(((. 1 .) 2 .) 3 (. 4 (. 5 (. 6 .))))");
    CHECK(!is_height_balanced(bad_inside.get()));
}

TEST(3, "rotation outcomes describe each unbalanced node") {
    const Tree tree = parse_tree("(. 1 (. 2 (. 3 .)))");
    const auto outcomes = rotation_outcomes(tree.get());
    REQUIRE_EQ(outcomes.size(), 1);
    // Node 1 has skew 2: rotating left balances the whole tree; rotating right is impossible.
    CHECK_EQ(outcomes[0], RotationOutcome{1, true, std::nullopt});
    CHECK_EQ(to_text(tree.get()), std::string("(. 1 (. 2 (. 3 .)))"));
}

TEST(3, "rotation outcomes can be false in the possible direction") {
    const Tree sample = parse_tree(kSample);
    const auto outcomes = rotation_outcomes(sample.get());
    REQUIRE_EQ(outcomes.size(), 1);
    // Left rotation at 70 leaves 90 over 70 over 80, still unbalanced; no left child to rotate right.
    CHECK_EQ(outcomes[0], RotationOutcome{70, false, std::nullopt});
    CHECK_EQ(to_text(sample.get()), std::string(kSample));
}

TEST(3, "a double-rotation case reports false for the single rotation") {
    const Tree tree = parse_tree("(. 1 ((. 2 .) 3 .))");
    const auto outcomes = rotation_outcomes(tree.get());
    REQUIRE_EQ(outcomes.size(), 1);
    CHECK_EQ(outcomes[0], RotationOutcome{1, false, std::nullopt});
}

TEST(3, "random trees agree with an independent rotation check and stay intact") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        INFO("round ", round);
        Tree tree;
        const auto n = static_cast<int>(rng.uniform(1, 25));
        for (int i = 0; i < n; ++i) {
            bst_insert(tree, static_cast<int>(rng.uniform(1, 100)));
        }
        const std::string before = to_text(tree.get());
        const auto reported = unbalanced_nodes(tree.get());
        const auto outcomes = rotation_outcomes(tree.get());
        REQUIRE_EQ(outcomes.size(), reported.size());
        CHECK_EQ(to_text(tree.get()), before);

        std::function<const TreeNode*(const TreeNode*, int)> find = [&](const TreeNode* node, int key) -> const TreeNode* {
            while (node != nullptr && node->key != key) {
                node = key < node->key ? node->left.get() : node->right.get();
            }
            return node;
        };
        for (std::size_t i = 0; i < outcomes.size(); ++i) {
            REQUIRE_EQ(outcomes[i].key, reported[i].first);
            const TreeNode* node = find(tree.get(), outcomes[i].key);
            const std::optional<bool> left =
                node->right == nullptr ? std::nullopt : std::optional<bool>(oracle_balanced_after(tree.get(), outcomes[i].key, true));
            const std::optional<bool> right =
                node->left == nullptr ? std::nullopt : std::optional<bool>(oracle_balanced_after(tree.get(), outcomes[i].key, false));
            REQUIRE_EQ(outcomes[i], RotationOutcome{outcomes[i].key, left, right});
        }
    }
}
