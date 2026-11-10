---
title: Binary Tree Practice (hand calculations as code)
unit: 04 · Trees & Heaps (PS4)
source: 6.006 PS4 Problem 4-1
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec06.pdf, materials/6.006/lectures/lec07.pdf
kind: code
minutes: 45
---

# 0411 · Binary Tree Practice

**Original problem:** [Problem Set 4 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/bff0aaeeb571a77083f10fd8e31f71d4_MIT6_006S20_ps4-questions.pdf) · **Read first:** [Lecture 6: Binary Trees, Part 1 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/376714cc85c6c784d90eec9c575ec027_MIT6_006S20_lec6.pdf), [Lecture 7: Binary Trees, Part 2: AVL notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/a2c80596cf4a2b5fbc854afdd2f23dcb_MIT6_006S20_lec7.pdf)

> **Why this comes up in practice:** when you write a balanced tree or a B-tree, the fastest way to debug it is to print the tree before and after a rotation and compare. This exercise turns the hand calculations into small tools that you can reuse later whenever you debug a tree structure.

## Original problem

6.006 PS4 Problem 4-1 works by hand on one binary search tree T, whose integer items are their own keys; T satisfies the BST property but is not height-balanced. The root of T is 47. Its left child is 16, whose children are 3 (left) and 37 (right); 37 has only a left child, 35, which has only a left child, 28. The root's right child is 84, whose children are 64 (left) and 86 (right); 64 has only a left child, 49, and 86 has only a right child, 88. In this exercise's text format, T is `(((. 3 .) 16 (((. 28 .) 35 .) 37 .)) 47 (((. 49 .) 64 .) 84 (. 86 (. 88 .))))`.

- (a) Find every node of T that is not height-balanced, and give its key and its skew.
- (b) Apply insert 2, delete 49, delete 35, insert 85, and delete 84 to T in that order, without any rotations: each operation adds or removes a leaf and keeps the BST property, so a key being deleted may first have to be swapped down to a leaf. Draw the tree after every operation.
- (c) For each node found in (a), draw the original T after rotating that node left and after rotating it right (whenever that rotation is possible), and say whether each of those trees is height-balanced, meaning that every node satisfies the AVL property.

## Your task

The original problem has you operate on a BST by hand. `support/tree_text.hpp` provides `TreeNode` (children held by `unique_ptr`), `Tree` (`unique_ptr<TreeNode>`), `parse_tree`, `to_text`, and `clone_tree`. `.` stands for an empty subtree and `(left key right)` for a node, so you can write any tree as a one-line string and print it out whenever you like. Here each hand step becomes a function: Stage 1 covers part (a), Stage 2 part (b), and Stage 3 part (c).

Once you're done, enter the tree from the Original problem section with `parse_tree`, run your code on it, and compare the results with Problem 4-1 in `answer`.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Height and skew

```cpp
struct TreeNode {  // provided by support/tree_text.hpp
    explicit TreeNode(int k) : key(k) {}
    int key;
    std::unique_ptr<TreeNode> left;
    std::unique_ptr<TreeNode> right;
};

using Tree = std::unique_ptr<TreeNode>;  // provided by support/tree_text.hpp

inline int subtree_height(const TreeNode* node);
inline int skew(const TreeNode* node);
inline std::vector<std::pair<int, int>> unbalanced_nodes(const TreeNode* root);
```

- `subtree_height(node)`: -1 for an empty tree, 0 for a leaf.
- `skew(node)`: `height(right) - height(left)`.
- `unbalanced_nodes(root)`: every node with `|skew| > 1`, returned as `(key, skew)` pairs in increasing order of key.

## Stage 2 — BST insertion and deletion without rotations

```cpp
inline void bst_insert(Tree& root, int key);
inline void bst_erase(Tree& root, int key);
```

Both modify `Tree& root` in place; assume it is a valid BST with distinct keys:

- `bst_insert(root, key)`: walk down the BST to an empty spot and add a new leaf there. If the key already exists, do nothing.
- `bst_erase(root, key)`: swap the key down to a leaf and then delete it (this is exactly the lecture's method, and it determines the shape of the result, so follow it): if the node has a left subtree, swap with its **predecessor**; otherwise swap with its **successor**; repeat until the key is in a leaf. If the key doesn't exist, do nothing.

## Stage 3 — Rotation outcomes

```cpp
inline void rotate_left_at(Tree& root, int key);
inline void rotate_right_at(Tree& root, int key);
inline bool is_height_balanced(const TreeNode* root);

struct RotationOutcome {
    int key;
    std::optional<bool> left;
    std::optional<bool> right;
    friend bool operator==(const RotationOutcome&, const RotationOutcome&) = default;
};

inline std::vector<RotationOutcome> rotation_outcomes(const TreeNode* root);
```

- `rotate_left_at(root, key)` / `rotate_right_at(root, key)`: rotate at the node that holds key. Throw `std::invalid_argument` if the key doesn't exist or if that direction can't be rotated, and leave the tree unchanged in that case.
- `is_height_balanced(root)`: whether every node satisfies `|skew| ≤ 1`.
- `rotation_outcomes(root)`: **must not modify `root`**. For each node that `unbalanced_nodes` finds (in increasing order of key), try one rotation to the left and one to the right (on a copy), and return `RotationOutcome{key, left, right}`: `left`/`right` is `std::nullopt` if that direction can't be rotated, and otherwise says whether the whole tree is height-balanced after the rotation.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does `bst_erase` always preserve the BST property? If the node being deleted has only one child, do you still need to swap?
2. What characterizes an unbalanced node that a single rotation can fix? When do you need a double rotation?
3. When you write a rotation with `unique_ptr`, which step is the easiest place to lose a subtree or double-free one?

---

**Official solution:** [Problem Set 4 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0462839ce975fea99f6a7b8adc8e1477_MIT6_006S20_ps4-solutions.pdf). Read it after you finish.
