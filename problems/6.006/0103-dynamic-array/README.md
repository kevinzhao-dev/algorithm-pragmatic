---
title: Dynamic Array（攤提分析與 Exception Safety）
unit: 01 · Sequences（PS1）
source: 6.006 Lecture 2, Recitation 2
pdf: materials/6.006/recitations/rec02.pdf
read: materials/6.006/lectures/lec02.pdf
kind: code
minutes: 120
---

# 0103 · Dynamic Array

> **實務上為什麼會遇到**：`std::vector` 是你寫 C++ 最常用的容器。尾端插入為什麼是攤提 O(1)、為什麼刪到快空了也不該馬上縮小、為什麼 `move` 不標 `noexcept` 會讓 `vector` 擴容時退回複製，都在這一題裡。

自己管理記憶體，**不可使用 `std::vector`、`std::deque`**。`T` 不一定能 default-construct，所以不能用 `new T[n]`：要用 `std::allocator`（或 `::operator new`）配置原始記憶體，再用 `std::construct_at` 把物件放進去，並確實呼叫解構子。

一次只看一關，做完再往下讀。

## Stage 1 — 基本操作與物件生命週期

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `DynamicArray()` | 空陣列，不可配置記憶體 | O(1) |
| `insert_last(v)` | 尾端新增 | 攤提 O(1) |
| `delete_last()` | 刪除並回傳尾端元素；空陣列丟 `std::out_of_range` | 攤提 O(1) |
| `size()`、`capacity()`、`empty()` | | O(1) |
| `get_at(i)`、`set_at(i, v)` | 超出範圍丟 `std::out_of_range` | O(1) |
| `operator[]`（兩個版本）、`begin()`/`end()` | 不檢查範圍 | O(1) |

- 只有前 `size()` 個位置有物件，**多餘的 capacity 裡不可以有任何建構好的物件**（測試用 `Tracked` 逐步檢查）。
- 被刪除的元素要真的被解構。Move-only 的元素（`std::unique_ptr`）也要能用。

## Stage 2 — 攤提分析的契約

`capacity()` 不綁定特定數值，但必須滿足：

1. 永遠 `size() ≤ capacity() ≤ 4·size() + 16`。
2. **連續 n 次 `insert_last` 的元素搬動總次數（copy + move）≤ 5n。**
3. 不管 `insert_last`、`delete_last` 怎麼交替（尤其在剛好滿、或剛好縮小的邊界），都不能發生反覆大量搬動（thrashing）。

Lecture 2 的做法：滿了就擴大成 2 倍；`size` 掉到 capacity 的 1/4 才縮小一半。想清楚為什麼「掉到 1/2 就縮小」不行。

## Stage 3 — Value Semantics 與中間操作

- Copy constructor、copy assignment（深拷貝，strong guarantee）、move constructor、move assignment（O(1)，`noexcept`，被搬走的物件 `size()` 為 0）。
- `insert_at(i, v)`：在索引 `i` 之前插入，`0 ≤ i ≤ size()`。容量足夠時只搬動 `i` 之後的元素。
- `delete_at(i)`：刪除並回傳第 `i` 個元素，一樣只搬動 `i` 之後的元素。
- `i` 超出範圍丟 `std::out_of_range`，內容不變。

## Stage 4 — Strong Exception Guarantee

`insert_last` 要和 `std::vector::push_back` 一樣提供 strong guarantee：**元素的 copy 或 move 在擴容途中丟例外時，陣列的內容與 `size()` 完全不變**，也不可洩漏。

- 擴容時，如果元素的 move constructor 是 `noexcept`，就用 move（否則擴容退化成複製會很貴）；如果 move 可能丟例外，就要改用 copy（`std::move_if_noexcept`）。
- 配置新記憶體要在動到舊資料之前，建好全部新元素再一次切換。

## 面試追問

1. 用 accounting method 說明 doubling 的攤提 O(1)：每次 `insert_last` 要存多少「錢」？
2. 為什麼 shrink 門檻不能和 grow 門檻太接近？給一個具體的最壞操作序列。
3. `std::vector::insert` 在中間插入時為什麼只提供 basic guarantee，尾端 `push_back` 卻是 strong？
4. 為什麼有些實作不用 2 倍而用 1.5 倍？這跟 allocator 重複使用舊區塊有什麼關係？
