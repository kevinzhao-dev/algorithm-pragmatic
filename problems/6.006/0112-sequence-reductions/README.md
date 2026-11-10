---
title: 在 Sequence 介面上做批次操作
unit: 01 · Sequences（PS1）
source: 6.006 PS1 Problem 1-2
pdf: materials/6.006/psets/ps1/questions.pdf
answer: materials/6.006/psets/ps1/solutions.pdf
read: materials/6.006/lectures/lec02.pdf
kind: code
minutes: 50
---

# 0112 · Sequence 上的批次操作

> **實務上為什麼會遇到**：許多資料結構（平衡樹版的 sequence、rope、collaborative editor 的文件模型）只提供 `insert_at` / `delete_at`，其他批次操作都要自己組出來，而且成本要跟「動多少東西」成正比，不能跟整個序列的長度成正比。

作業給定一個 Sequence，只有 `insert_at` 和 `delete_at` 可用，要你用這兩個操作組出區段反轉與區段搬移，每個操作成本 O(log n) 時總成本要 O(k log n)。這裡把「每次操作 O(log n)」換成**直接計算呼叫次數**：測試會傳入一個計數用的序列，你的函式只能呼叫它的這三個操作：

```cpp
seq.size();
seq.insert_at(i, std::move(item));   // 0 ≤ i ≤ seq.size()，在索引 i 之前插入
auto item = seq.delete_at(i);        // 刪除並回傳，0 ≤ i < seq.size()
```

所有函式都是 template（`Seq` 只保證有這三個操作），元素只保證能 move（`std::unique_ptr` 也要能用）。參數不合法時丟 `std::invalid_argument`，序列不變。

## Stage 1 — `reverse_range(seq, i, k)`

把從索引 `i` 開始的連續 `k` 個元素反轉。

- 需要 `i + k ≤ seq.size()`。`k ≤ 1` 時什麼都不用做。
- `insert_at` + `delete_at` 的**總呼叫次數 ≤ 2k**。

## Stage 2 — `move_range(seq, i, k, j)`

把從索引 `i` 開始的 `k` 個元素，依原本順序搬到「目前索引 `j` 的元素」的前面。`j` 用**搬移前**的索引。

- 需要 `i + k ≤ seq.size()`，`j ≤ seq.size()`（`j == seq.size()` 代表搬到最後），且 `j` 不能落在區段內部（`i ≤ j < i + k` 不成立）。`k == 0` 不做事。
- 總呼叫次數 **≤ 2k**。難點是每次 delete 之後索引都會平移，要想清楚 `j` 在往左搬與往右搬時各該怎麼調整。

## Stage 3 — `rotate_left(seq, i, k, m)`

把從 `i` 開始的 `k` 個元素往左旋轉 `m` 格（區段的前 `m` 個元素移到區段末尾，`0 ≤ m ≤ k`，否則丟 `std::invalid_argument`）。

用 `move_range` 組出來，並且**挑便宜的方向**：總呼叫次數 ≤ 2·min(m, k − m)。

## 面試追問

1. 假設序列的 `insert_at`、`delete_at` 是 O(log n)（例如 AVL），為什麼 `reverse_range` 不用平衡樹的特殊操作就能做到 O(k log n)？如果 `k` 接近 `n`，有沒有更快的辦法？
2. `move_range` 在 `j > i` 和 `j < i` 兩種情況，索引平移為什麼不同？用不變量（invariant）說明你的迴圈。
3. 如果序列支援 O(log n) 的 split / concatenate（例如 treap），`move_range` 的成本可以怎麼改寫？
