---
title: Binary Tree 與 Traversal Order
unit: 04 · Trees & Heaps（PS4）
source: 6.006 Lecture 6, Recitation 6
pdf: materials/6.006/recitations/rec06.pdf
read: materials/6.006/lectures/lec06.pdf
kind: code
minutes: 100
---

# 0401 · Binary Tree 與 Traversal Order

> **實務上為什麼會遇到**：`std::map`、資料庫的 B+Tree、OS 的 VMA tree 都建立在同一個想法上：節點的中序走訪定義了順序，把「改變集合」化為局部的指標操作。這題把這套基本功練到不用想。

這題是之後 AVL、Set、Sequence 的共同基礎。實作有擁有權的 `BinaryTree<T>`（節點含 `parent`、`left`、`right`），操作全部用 **traversal order（中序）** 來思考；這一題**不做平衡**。

一次只看一關。

## Stage 1 — 導航（O(h)）

都是 static 函式，對任何節點都能呼叫：

- `subtree_first(n)` / `subtree_last(n)`：`n` 子樹在 traversal order 中第一個／最後一個節點。`n` 為 `nullptr` 時回傳 `nullptr`。
- `successor(n)` / `predecessor(n)`：整棵樹的下一個／上一個節點，沒有時回傳 `nullptr`。

## Stage 2 — 插入與走訪

| 操作 | 行為 |
| --- | --- |
| `insert_before(a, v)` | `a` 沒有左子節點時，新節點成為 `a` 的左子節點；否則成為 `a` 左子樹最後一個節點的右子節點 |
| `insert_after(a, v)` | 左右對稱 |
| `insert_before(nullptr, v)` / `insert_after(nullptr, v)` | 分別表示接到整棵樹的尾端／開頭 |
| `root()` / `size()` / `empty()` | |
| `begin()` / `end()` | forward iterator，走完整棵樹 O(n)（`std::forward_iterator` 會被檢查） |

節點 `new` 出來後歸樹所有。解構子要釋放全部節點，且**不可以遞迴**：一條退化成 linked list 的樹，遞迴解構會把 stack 撐爆。

## Stage 3 — 刪除

`erase(node)`：把 `node` 從樹中移除，回傳它的值，並維持其餘節點的 traversal order。O(h)。

**只有被刪除的 `node` 失效，其他所有 `Node*` 必須仍然指向同一個 item。** 所以不能像 lecture 的 Python 版本那樣把 item 跟 successor 交換；要用指標把節點搬到正確位置（`successor` 是你手上剛好有的節點，比搬 item 乾淨）。

邊界：葉節點、只有一個子節點、有兩個子節點、刪根、`successor` 正好是右子節點。

## Stage 4 — O(n) 建樹

- `static BinaryTree build(std::vector<T> items)`：建出 traversal order 等於 `items` 的樹，O(n)，高度為 `⌊lg n⌋`（空樹除外），用 move 把 item 搬進節點。
- move constructor / move assignment：被移走的樹變空，要 `noexcept`。

## Stage 5 — Set 介面：Binary Search Tree

用 `BinaryTree<K>` 實作 `BstSet<K, Compare>`：traversal order 就是排序好的 key 順序。每個操作 O(h)：

- `insert(k)`（新 key 回傳 `true`）、`contains(k)`、`erase(k)`（有刪到回傳 `true`）、`size()`、`empty()`。
- `find_min()` / `find_max()`：回傳 `std::optional<K>`。
- `find_next(k)`：最小的 `> k` 的 key；`find_prev(k)`：最大的 `< k` 的 key。`k` 本身不必存在。
- `to_vector()`：排序好的所有 key。`height()`：空樹 `-1`，否則是根到最深葉節點的邊數（可以 O(n)）。

記住這棵樹會退化：依序插入遞增的 key 時，高度是 `n - 1`。下一題解決這件事。

## 面試追問

1. 為什麼 `successor` 的最壞情況是 O(h)，而走訪整棵樹卻只要 O(n)？
2. Lecture 的 delete 用 swap item 寫起來簡單，C++ 上換成 pointer relink 的代價和好處是什麼？
3. `std::set` 的 iterator 在 `erase` 後還能用嗎？你的設計能不能達到一樣的保證？
