#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "tree_text.hpp"

// Stage 1
inline int subtree_height(const TreeNode* node) { PRACTICE_TODO(); }
inline int skew(const TreeNode* node) { PRACTICE_TODO(); }
inline std::vector<std::pair<int, int>> unbalanced_nodes(const TreeNode* root) { PRACTICE_TODO(); }

// Stage 2
inline void bst_insert(Tree& root, int key) { PRACTICE_TODO(); }
inline void bst_erase(Tree& root, int key) { PRACTICE_TODO(); }

// Stage 3
inline void rotate_left_at(Tree& root, int key) { PRACTICE_TODO(); }
inline void rotate_right_at(Tree& root, int key) { PRACTICE_TODO(); }
inline bool is_height_balanced(const TreeNode* root) { PRACTICE_TODO(); }

struct RotationOutcome {
    int key;
    std::optional<bool> left;
    std::optional<bool> right;
    friend bool operator==(const RotationOutcome&, const RotationOutcome&) = default;
};

inline std::vector<RotationOutcome> rotation_outcomes(const TreeNode* root) { PRACTICE_TODO(); }
