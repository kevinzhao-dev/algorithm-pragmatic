---
title: Binary Tree Practice（程式化手算）
unit: 04 · Trees & Heaps（PS4）
source: 6.006 PS4 Problem 4-1
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec06.pdf, materials/6.006/lectures/lec07.pdf
kind: code
minutes: 45
---

# 0411 · Binary Tree Practice

> **實務上為什麼會遇到**：寫平衡樹或 B-tree 時，最快的除錯方式就是把旋轉前後的樹印出來對照。這題把手算的動作寫成小工具；之後你除錯樹結構時可以直接拿來用。

原題要手動在一棵 BST 上操作。`support/tree_text.hpp` 提供了 `TreeNode`（`unique_ptr` 持有子節點）、`Tree`（`unique_ptr<TreeNode>`）、`parse_tree`、`to_text`、`clone_tree`，`.` 表示空子樹，`(left key right)` 表示節點，隨手就能把樹寫成一行字串、印出來看。

寫完之後，把 PDF 第 1 頁的樹用 `parse_tree` 打進去跑，對照 `answer` 的 Problem 4-1。

## Stage 1 — 高度與 skew

- `subtree_height(node)`：空樹是 -1，葉節點是 0。
- `skew(node)`：`height(right) - height(left)`。
- `unbalanced_nodes(root)`：所有 `|skew| > 1` 的節點，以 `(key, skew)` 的形式依 key 遞增回傳。

## Stage 2 — 不旋轉的 BST 插入與刪除

都在原地修改 `Tree& root`，假設是合法的 BST，key 互異：

- `bst_insert(root, key)`：沿 BST 走到空位新增葉節點；key 已存在時不動作。
- `bst_erase(root, key)`：把 key 往下換到葉節點再刪掉（這正是 lecture 的做法，決定了結果的形狀，所以要照做）：有左子樹時跟**前驅**交換，否則跟**後繼**交換，重複直到它變成葉節點。key 不存在時不動作。

## Stage 3 — 旋轉結果

- `rotate_left_at(root, key)` / `rotate_right_at(root, key)`：在 key 所在節點旋轉。key 不存在、或該方向無法旋轉時丟 `std::invalid_argument`。
- `is_height_balanced(root)`：是否所有節點都滿足 `|skew| ≤ 1`。
- `rotation_outcomes(root)`：**不可修改 `root`**。對 `unbalanced_nodes` 找到的每個節點（依 key 遞增），試著分別往左／往右旋轉一次（在複本上），回傳 `RotationOutcome{key, left, right}`：`left`／`right` 是 `std::nullopt` 表示該方向無法旋轉，否則是「旋轉後整棵樹是否 height-balanced」。

## 面試追問

1. `bst_erase` 為什麼一定保持 BST 性質？刪掉的節點如果只有一個子節點，還需要 swap 嗎？
2. 只靠一次旋轉就能修好的不平衡節點有什麼特徵？什麼情況需要雙旋？
3. 用 `unique_ptr` 寫旋轉時，哪個步驟最容易把子樹弄丟或 double-free？
