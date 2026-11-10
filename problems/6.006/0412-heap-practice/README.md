---
title: Heap Practice（分類與 swap trace）
unit: 04 · Trees & Heaps（PS4）
source: 6.006 PS4 Problem 4-2
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec08.pdf, materials/6.006/recitations/rec08.pdf
kind: code
minutes: 30
---

# 0412 · Heap Practice

> **實務上為什麼會遇到**：heap 相關的 bug 通常不是 crash，而是順序悄悄錯掉。這題練的是把「一個陣列是不是 heap」變成機械化的檢查，也順便練習建立 heap 的每一步。

把陣列視為完全二元樹（`left(i) = 2i+1`、`right(i) = 2i+2`）。做完後可以把 PDF 上那幾組陣列拿來驗算。

## Stage 1 — 分類

`classify_heap(values)`：回傳 `HeapKind::Max`、`Min`、`Both`（空陣列、單一元素、或全部相同）或 `Neither`。允許相等，例如 `[5, 5, 3]` 是 max-heap。

## Stage 2 — 建 min-heap 並記錄 swap

`min_heapify_trace(values)`：**在原地**把 `values` 轉成 min-heap，回傳每一次交換的索引對 `(i, j)`。

- 每次交換的兩個位置必須在樹上相鄰（一個是另一個的父節點），且是對當下陣列的實際交換，也就是**只**靠這些 swap 就能從原陣列得到結果。
- 已經是 min-heap 就回傳空列表。
- 交換次數不能超過 `n`（`n = values.size()`）。

測試會把你的 trace 重放在原陣列上驗證。

## Stage 3 — 最小值與轉換

- `min_of_max_heap(heap)`：`heap` 是 max-heap，回傳其中一個最小元素的參考（空的時候丟 `std::invalid_argument`）。比較次數不能超過 `⌈n/2⌉`。想一下最小值只能在哪些位置。
- `max_heap_to_min_heap(heap)`：把 max-heap 改造成 min-heap，比較次數 O(n)。（max-heap 這個前提有沒有幫到你？）

模板參數 `T` 只需要支援 `<`。

## 面試追問

1. Stage 2 用「由下往上 sift-down」跟「依序 sift-up」的 swap 次數差多少？
2. `min_of_max_heap` 為什麼不能少於約 `n/2` 次比較？
3. 把 max-heap 轉成 min-heap，為什麼已知的 max-heap 結構完全幫不上忙？
