---
title: Binary Heap 與 In-Place Heapsort
unit: 04 · Trees & Heaps（PS4）
source: 6.006 Lecture 8, Recitation 8
pdf: materials/6.006/recitations/rec08.pdf
read: materials/6.006/lectures/lec08.pdf
kind: code
minutes: 75
---

# 0405 · Binary Heap 與 In-Place Heapsort

> **實務上為什麼會遇到**：排程器、Dijkstra、TCP 重組緩衝、Top-K，全都是 priority queue。Linux 的 hrtimer 用紅黑樹，但 Go runtime 的 timer 就是四叉堆；陣列隱式樹的 cache 表現是 pointer-based 結構比不上的。

把陣列看成隱式的完全二元樹（`parent(i) = (i-1)/2`、`left(i) = 2i+1`、`right(i) = 2i+2`），用 `std::vector` 當底層，不准用 `std::priority_queue`、`std::push_heap` 等。

`BinaryHeap<T, Compare>` 是**最大堆**（依 `Compare`：`top()` 是使 `Compare` 判定為最大的元素）；傳 `std::greater<>` 就成了最小堆。

## Stage 1 — 基本操作

- `push(v)`：O(log n)。
- `top()` / `pop()`：取得／移除堆頂（`pop` 回傳值）。空堆時丟 `std::out_of_range`。`pop` O(log n)。
- `size()` / `empty()`。
- `items()`：底層陣列（唯讀的 `std::span`），`items()[0]` 是堆頂。測試用它檢查 heap property。

元素可以是 move-only，`pop` 用 move 回傳。

## Stage 2 — O(n) 建堆

`explicit BinaryHeap(std::vector<T> items, Compare = {})`：**由下往上** sift-down，O(n) 建堆。比較次數不能超過 `3n`（「一個一個 push」要 O(n log n)，測試會數）。

先想一下為什麼 sift-down 建堆是 O(n)，而 sift-up 不是：葉節點佔一半，它們幾乎不用動。

## Stage 3 — In-Place Heapsort

`heapsort(first, last, compare = {})`：把 random access iterator 範圍排成 `compare` 由小到大。

- O(1) 額外空間：不能把元素複製到別的容器（測試會用 `Tracked` 看活著的物件數量）。不要求穩定。
- 比較次數 O(n log n)。
- 可以和 Stage 1 的 sift 函式共用程式碼：把 heap 操作拆成吃 iterator 的 free function，就能同時服務 `BinaryHeap` 和 `heapsort`。

## Stage 4 — 實務延伸：亂序事件重排

串流裡的事件可能晚到一點點（網路封包、log 匯流）。`ReorderBuffer<T, Compare>`：

- `explicit ReorderBuffer(lateness)`：保證每個事件距離它排序後的位置最多 `lateness` 格（`lateness = 0` 表示已經排好）。
- `push(item)`：放入事件；緩衝區超過 `lateness` 個事件時，回傳目前最早的事件（依 `Compare` 最小），否則回傳 `std::nullopt`。
- `drain()`：串流結束時，依序回傳緩衝區剩下的事件。
- `buffered()`：目前緩衝的數量，永遠不超過 `lateness`。

所有輸出的連接必須是完整的排序結果。每個 `push` 的比較次數 O(log(lateness + 1))（而不是 O(log n)），這就是 heap 大小只留 `lateness + 1` 的好處。

## 面試追問

1. 找最大堆裡的最小元素要多久？為什麼不能更快？
2. Heapsort 跟 mergesort、quicksort 比，常數和快取行為有什麼差別？為什麼 std::sort 不用 heapsort 當主力，卻用它當保底？
3. 想要支援 `decrease-key`，這個結構還缺什麼？（下一題）
