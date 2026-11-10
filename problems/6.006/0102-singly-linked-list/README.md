---
title: Singly Linked List 與 Floyd 環偵測
unit: 01 · Sequences（PS1）
source: 6.006 Recitation 2
pdf: materials/6.006/recitations/rec02.pdf
read: materials/6.006/lectures/lec02.pdf
kind: code
minutes: 80
---

# 0102 · Singly Linked List

> **實務上為什麼會遇到**：free list、hash table 的 bucket 鏈、kernel 的 task list 都是單向鏈結串列。而鏈結串列最常見的 bug 之一就是指標形成環，走訪永遠停不下來，這正是 Floyd 演算法要解決的問題。

一次只看一關，做完再往下讀。

## Stage 1 — 節點與所有權

`LinkedListSeq<T>` 擁有自己的節點，`Node` 的結構已經寫在骨架裡。

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `LinkedListSeq()` / `explicit LinkedListSeq(span<const T>)` | 空串列 / 依序建立 | O(1) / O(n) |
| `insert_first(v)` | 插入到最前面，回傳新節點 | O(1) |
| `delete_first()` | 移除並回傳最前面的值；空串列丟 `std::out_of_range` | O(1) |
| `head()`、`size()`、`empty()` | | O(1) |
| 解構子 | 釋放所有節點，不可遞迴 | O(n) |

Copy 已禁止；補上 move constructor 與 move assignment（被搬走的串列要變成空的、size 為 0）。

## Stage 2 — 索引操作與迭代器

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `get_at(i)` / `set_at(i, v)` | 讀寫第 `i` 個元素；超出範圍丟 `std::out_of_range` | O(i) |
| `insert_at(i, v)` | 在索引 `i` 之前插入，`0 ≤ i ≤ size()`，回傳新節點 | O(i) |
| `delete_at(i)` | 刪除並回傳第 `i` 個元素 | O(i) |
| `insert_last(v)` / `delete_last()` | 尾端操作（這個串列沒有 tail，O(n) 沒關係） | O(n) |

失敗的操作不可改變串列內容。

再補上 STL 風格的 forward iterator：`iterator` 與 `const_iterator`（可以用 range-for、`std::find`），並要滿足 `std::forward_iterator` concept。`begin()`/`end()` 各有 `const` 與非 `const` 版本。

## Stage 3 — 原地反轉

`reverse()`：把串列整個反轉。**O(n) 時間、O(1) 額外空間，只改指標**：節點不可重建、元素不可複製或搬動（測試會檢查節點位址與元素的 move 次數）。不可遞迴。

## Stage 4 — Floyd 環偵測

下面這個函式作用在**任意帶有 `next` 指標的節點型別**上（不是你的串列）。`head` 可能是 `nullptr`，節點的 `next` 鏈可能形成一個環（ρ 形：一段尾巴接上一個環）。

```cpp
template <typename Node>
struct CycleInfo { std::size_t length; const Node* entry; };

std::optional<CycleInfo<Node>> find_cycle(const Node* head);
```

沒有環時回傳 `std::nullopt`；有環時回傳環上的節點數，以及從 `head` 走進環時遇到的**第一個**環上節點。

要求 **O(n) 時間、O(1) 額外空間**（測試會確認沒有任何 heap allocation，也就是不能用 visited set）。

## 面試追問

1. 為什麼快慢指標一定會相遇？相遇點和環的入口有什麼關係，第二階段為什麼會在入口相會？
2. 用 `std::unique_ptr<Node>` 當 `next` 時，解構子為什麼可能 stack overflow？要怎麼解？
3. 什麼情況下鏈結串列真的會比 `std::vector` 快？現代 CPU 的快取會怎麼影響這個判斷？
