---
title: AVL Tree 引擎（rotation 與 subtree augmentation）
unit: 04 · Trees & Heaps（PS4）
source: 6.006 Lecture 7, Recitation 7
pdf: materials/6.006/recitations/rec07.pdf
read: materials/6.006/lectures/lec07.pdf
kind: code
minutes: 150
---

# 0402 · AVL Tree 引擎

> **實務上為什麼會遇到**：每個 balanced tree（AVL、Red-Black）的核心都只有「旋轉 + 從葉往根修復」。學會這個之後，order statistics、線段查詢這些需求都只是「多維護一個 subtree 的摘要」而已，這種 augmentation 是資料結構題最常見的加分招式。

把 0401 的 `BinaryTree` 複製過來，一路加上高度、旋轉和平衡。下兩題的 Set 和 Sequence 都建立在這個引擎上，後面的題目也會重用它，所以介面請照規格。

一次只看一關。

## Stage 1 — 節點帶高度與大小

介面和 0401 相同（`root`、`size`、`empty`、`begin`/`end`、`subtree_first/last`、`successor/predecessor`、`insert_before/after`、`erase`），`insert_before/after` 的 `nullptr` anchor 語意也相同。但 `Node` 多了兩個欄位，**每次操作之後對每個節點都要正確**：

- `height`：leaf 是 0，空子樹視為 -1。
- `size`：這棵子樹的節點數。

`erase` 同樣只能讓被刪的節點失效，不可 swap item。這一關還不用平衡。

## Stage 2 — Rotation

`rotate_left(x)`（要求 `x` 有右子節點）/ `rotate_right(x)`（要求有左子節點）：O(1)，回傳取代 `x` 位置的節點，`traversal order` 不變，兩個被搬動節點的 `height`、`size`（以及 Stage 4 的 `aug`）要重算。祖先由呼叫者負責，這一步不需要更新。

## Stage 3 — 維持平衡

每個節點都要 height-balanced：左右子樹高度差最多 1（skew ∈ {-1, 0, 1}）。

每次 `insert_before/after` 或 `erase` 之後，從變動的最低點往根走，用一兩次旋轉修復（lecture 的 case 1/2 單旋，case 3 雙旋），並沿路更新 `height`、`size`。`Node*` 必須在旋轉後繼續有效。

- 所有操作 O(log n)。
- 依序插入 `n` 個元素，高度必須 ≤ `1.45 lg(n + 2)`。

## Stage 4 — Subtree Augmentation

第二個模板參數 `Aug` 描述要在每個子樹上維護的摘要：

```cpp
struct SumAug {
    using Value = long long;                   // 每個節點存一份
    static Value identity() { return 0; }      // 空子樹
    static Value of(const T& item);            // 單一 item 的摘要
    static Value combine(const Value& a, const Value& b);  // 不一定交換：a 在 traversal order 上較前
};
```

- 對每個節點：`node->aug == combine(combine(aug(left), of(item)), aug(right))`，左右缺席時用 `identity()`。**`combine` 不保證交換律**（字串串接、矩陣相乘），順序寫錯就過不了。
- 每次 `insert`/`erase`/`rotate` 都維持這個不變量，而且只能呼叫 O(log n) 次 `combine`，不可以整棵重算。
- `refresh(node)`：呼叫者直接改了 `node->item` 之後，把 `node` 到根路徑上的 `aug` 重算（O(log n)，高度、大小不變）。

## Stage 5 — O(n) 建樹與所有權

- `static AvlTree build(std::vector<T> items)`：建出 traversal order 等於 `items` 的 AVL tree，O(n)。`combine` 呼叫次數不超過 `3n`。
- move constructor / move assignment：被移走的樹變空，要 `noexcept`。

## 面試追問

1. AVL 的高度上界為什麼是 `1.44 lg n`？（寫出 Fibonacci 遞迴）跟 Red-Black 比，AVL 的取捨是什麼？
2. 插入最多旋轉幾次？刪除為什麼可能要旋轉 O(log n) 次？
3. 為什麼 rotation 不改變 traversal order？實作上為什麼一定要**先**更新子節點的欄位，再更新父節點？
