#pragma once

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "tree_text.hpp"

inline int subtree_height(const TreeNode* node) {
    if (node == nullptr) {
        return -1;
    }
    return 1 + std::max(subtree_height(node->left.get()), subtree_height(node->right.get()));
}

inline int skew(const TreeNode* node) {
    return subtree_height(node->right.get()) - subtree_height(node->left.get());
}

namespace detail {

inline void collect_unbalanced(const TreeNode* node, std::vector<std::pair<int, int>>& out) {
    if (node == nullptr) {
        return;
    }
    collect_unbalanced(node->left.get(), out);
    const int s = skew(node);
    if (s < -1 || s > 1) {
        out.emplace_back(node->key, s);
    }
    collect_unbalanced(node->right.get(), out);
}

inline Tree* find_slot(Tree& root, int key) {
    Tree* slot = &root;
    while (*slot != nullptr && (*slot)->key != key) {
        slot = key < (*slot)->key ? &(*slot)->left : &(*slot)->right;
    }
    return *slot == nullptr ? nullptr : slot;
}

}  // namespace detail

// Pairs (key, skew) of every node whose skew magnitude exceeds 1, in key order.
inline std::vector<std::pair<int, int>> unbalanced_nodes(const TreeNode* root) {
    std::vector<std::pair<int, int>> out;
    detail::collect_unbalanced(root, out);
    return out;
}

inline void bst_insert(Tree& root, int key) {
    Tree* slot = &root;
    while (*slot != nullptr) {
        if (key == (*slot)->key) {
            return;
        }
        slot = key < (*slot)->key ? &(*slot)->left : &(*slot)->right;
    }
    *slot = std::make_unique<TreeNode>(key);
}

// Swaps the key down to a leaf (predecessor when there is a left subtree, otherwise successor)
// and removes that leaf: the exact procedure the by-hand exercise uses.
inline void bst_erase(Tree& root, int key) {
    Tree* slot = detail::find_slot(root, key);
    if (slot == nullptr) {
        return;
    }
    while ((*slot)->left != nullptr || (*slot)->right != nullptr) {
        TreeNode* node = slot->get();
        Tree* next = nullptr;
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
    slot->reset();
}

inline void rotate_left_at(Tree& root, int key) {
    Tree* slot = detail::find_slot(root, key);
    if (slot == nullptr || (*slot)->right == nullptr) {
        throw std::invalid_argument("cannot rotate left");
    }
    Tree pivot = std::move((*slot)->right);
    (*slot)->right = std::move(pivot->left);
    pivot->left = std::move(*slot);
    *slot = std::move(pivot);
}

inline void rotate_right_at(Tree& root, int key) {
    Tree* slot = detail::find_slot(root, key);
    if (slot == nullptr || (*slot)->left == nullptr) {
        throw std::invalid_argument("cannot rotate right");
    }
    Tree pivot = std::move((*slot)->left);
    (*slot)->left = std::move(pivot->right);
    pivot->right = std::move(*slot);
    *slot = std::move(pivot);
}

inline bool is_height_balanced(const TreeNode* root) { return unbalanced_nodes(root).empty(); }

struct RotationOutcome {
    int key;
    std::optional<bool> left;   // empty: rotating left is impossible at this node
    std::optional<bool> right;
    friend bool operator==(const RotationOutcome&, const RotationOutcome&) = default;
};

inline std::vector<RotationOutcome> rotation_outcomes(const TreeNode* root) {
    std::vector<RotationOutcome> out;
    for (const auto& [key, skew_value] : unbalanced_nodes(root)) {
        RotationOutcome outcome{key, std::nullopt, std::nullopt};
        Tree left_copy = clone_tree(root);
        Tree right_copy = clone_tree(root);
        Tree* left_slot = detail::find_slot(left_copy, key);
        if ((*left_slot)->right != nullptr) {
            rotate_left_at(left_copy, key);
            outcome.left = is_height_balanced(left_copy.get());
        }
        Tree* right_slot = detail::find_slot(right_copy, key);
        if ((*right_slot)->left != nullptr) {
            rotate_right_at(right_copy, key);
            outcome.right = is_height_balanced(right_copy.get());
        }
        out.push_back(outcome);
    }
    return out;
}
