---
title: Static Array 與 Array Sequence
unit: 01 · Sequences（PS1）
source: 6.006 Lecture 1, Recitation 1-2
pdf: materials/6.006/recitations/rec02.pdf
read: materials/6.006/lectures/lec01.pdf, materials/6.006/lectures/lec02.pdf, materials/6.006/recitations/rec01.pdf
kind: code
minutes: 70
---

# 0101 · Static Array 與 Array Sequence

> **實務上為什麼會遇到**：`std::vector` 底層就是一段連續記憶體加上「長度」。在 RAM 模型裡，索引存取 O(1) 是因為位址可以算出來；而中間插入 O(n) 是因為後面的元素全部要搬。這題從最底層把這件事親手做一次。

自己管理記憶體，**不可使用 `std::vector`** 當底層容器。

## Stage 1 — StaticArray（Rule of Five）

`StaticArray<T>`：固定長度、擁有自身元素（`T` 可以 default-construct 與賦值）。

| 操作 | 行為 |
| --- | --- |
| `explicit StaticArray(n)` | `n` 個 value-initialized 的元素（`int` 為 0） |
| `size()` | 長度，O(1) |
| `get_at(i)`、`set_at(i, x)` | O(1)；`i` 超出範圍丟 `std::out_of_range` |
| `operator[]`（`const` 與非 `const`） | O(1)，不檢查範圍 |
| `begin()` / `end()` | 指標即可（讓 range-for 能用） |

資源管理：

- 解構子釋放所有元素。Copy 是深拷貝；copy assignment 要有 **strong guarantee**（元素複製丟例外時，原物件不受影響）。
- Move 必須 O(1)、不動到任何元素（搶指標就好），`noexcept`，被搬走的物件 `size()` 為 0。
- 自我賦值 `a = a` 必須正常。

## Stage 2 — ArraySequence

用 Stage 1 的 `StaticArray` 實作 Lecture 2 的 Sequence interface。陣列永遠剛好是滿的，所以每次新增或刪除都要**重新配置**並搬移元素：

| 操作 | 行為 |
| --- | --- |
| `ArraySequence()` / `explicit ArraySequence(span<const T>)` | 空的 / 複製建構，O(n) |
| `size()`、`get_at(i)`、`set_at(i, x)` | O(1)，超出範圍丟 `std::out_of_range` |
| `insert_at(i, x)` | 在索引 `i` 之前插入，`0 ≤ i ≤ size()`，O(n) |
| `delete_at(i)` | 刪除並回傳第 `i` 個元素，O(n) |
| `insert_first` / `insert_last` / `delete_first` / `delete_last` | 用上面的操作組合 |
| `begin()` / `end()` | 迭代 |

`insert_at` 和 `delete_at` 要做到 strong guarantee：元素複製丟例外、或 index 不合法時，序列內容不變。`ArraySequence` 本身應該用不到自訂的解構子或 copy / move（Rule of Zero）。

## Stage 3 — Lecture 1 的第一個演算法

`first_collision(records, key_of)`：依序檢查每筆資料，回傳 `(i, j)`，其中 `j` 是**最小的**「前面有資料具有相同 key」的索引，`i` 是其中最早的那一筆；沒有重複時回傳 `std::nullopt`。

用 Stage 1 的 `StaticArray` 當紀錄，O(n²) 即可（用 sort 或 hashing 是之後的章節）。

## 面試追問

1. `std::vector` 的 `operator[]` 為什麼不做範圍檢查，`at()` 要做？對效能的影響是什麼？
2. 為什麼這裡的 `insert_at` 是 O(n)，換成 dynamic array（0103）就能讓尾端插入攤提 O(1)？差別在哪裡？
3. 用歸納法說明 `first_collision` 的迴圈是正確的（提示：k 筆之內有沒有碰撞）。
