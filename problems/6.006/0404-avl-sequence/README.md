---
title: Sequence AVL（索引與區間 fold）
unit: 04 · Trees & Heaps（PS4）
source: 6.006 Lecture 7, Recitation 7
pdf: materials/6.006/recitations/rec07.pdf
read: materials/6.006/lectures/lec06.pdf, materials/6.006/lectures/lec07.pdf
kind: code
minutes: 80
---

# 0404 · Sequence AVL

> **實務上為什麼會遇到**：文字編輯器的 rope、需要「在中間插入又能隨機存取」的序列，都用這種結構。陣列的 `insert_at` 是 O(n)，linked list 的 `get_at` 是 O(n)；這棵樹兩個都是 O(log n)。

用 0402 的 `AvlTree` 實作 **Sequence**：這次 traversal order 就是序列順序，跟 key 無關。請 `#include "../0402-avl-tree/solution.hpp"`，重用你的引擎，不要重寫平衡邏輯。

```cpp
template <typename T, typename Aug = NoAugment>
class AvlSequence;
```

## Stage 1 — 建構與索引

- `explicit AvlSequence(std::vector<T> items)`：O(n) 建樹（用 0402 的 `build`）。
- `node_at(i)`：第 `i` 個節點（0 起算），O(log n)，`i >= size()` 時丟 `std::out_of_range`；這是其他操作的核心。
- `at(i)` / `set_at(i, v)`：O(log n)；`set_at` 要維持 augmentation。
- `size()` / `empty()` / `to_vector()` / `tree()`。

## Stage 2 — 中間插入與刪除

- `insert_at(i, v)`：`0 ≤ i ≤ size()`，把 `v` 放到第 `i` 個位置（原本 `i` 之後的元素往後移），`i > size()` 時丟 `std::out_of_range`。
- `delete_at(i)`：移除並回傳第 `i` 個，`i >= size()` 時丟 `std::out_of_range`。

都是 O(log n)。

## Stage 3 — 區間 fold

`fold(l, r)`：對區間 `[l, r)` 的 `Aug` 摘要做 fold，依序列順序 `combine`，O(log n)。`l == r` 回傳 `identity()`；`l > r` 或 `r > size()` 時丟 `std::out_of_range`。

**`combine` 不保證交換**，左右順序是這一關的陷阱。節點已經存了整棵子樹的摘要，不需要走訪區間內每個元素：完全包住的子樹直接用 `node->aug`，每一層最多再往下走兩個分支。呼叫 `combine` 的次數要是 O(log n)。

Recitation 7 的 `flip(i)`／`count_ones_upto(i)` 就是這個機制（`Aug` 是 bit 的總和）。

## Stage 4 — 實務延伸：文字緩衝區

`TextBuffer`（可以用 `AvlSequence<char, YourHashAug>` 實作）：

- `TextBuffer(text)`：O(n) 建立。`size()`、`at(i)`。
- `insert(i, ch)`、`erase(i)`：O(log n)。
- `substr(l, r)`：`[l, r)` 的內容，O(r − l + log n)，範圍非法丟 `std::out_of_range`。
- `equal_ranges(a, b, length)`：`[a, a+length)` 與 `[b, b+length)` 內容是否相同，**O(log n)**，範圍非法丟 `std::out_of_range`。

提示：多項式 hash 也是一個 monoid。值存 `(hash, base^len)`，`combine` 就是 `(h_a · p_b + h_b, p_a · p_b)`，全部 mod 一個大質數（例如 `2^61 - 1`）。`equal_ranges` 比較兩段的摘要，回答可能出錯的機率夠小就好。

## 面試追問

1. 為什麼 `fold` 只需要走 O(log n) 個節點？完全被 `[l, r)` 蓋住的節點有什麼特別？
2. 如果區間操作還要支援「整段加上一個數」，要改哪裡？（這就是 lazy propagation）
3. `equal_ranges` 回傳 `true` 時，實際一定相等嗎？怎麼降低誤報？誤報的代價是什麼？
