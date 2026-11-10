---
title: Selection Sort、Insertion Sort 與 Top-K Tracker
unit: 02 · Sorting & Recurrences（PS2）
source: 6.006 Lecture 3, Recitation 3
pdf: materials/6.006/lectures/lec03.pdf
read: materials/6.006/recitations/rec03.pdf
kind: code
minutes: 55
---

# 0202 · 基本排序

> **實務上為什麼會遇到**：標準函式庫裡的 sort 幾乎都會在小區間換成 insertion sort（資料少、幾乎排好時它最快）；Selection sort 的「元素搬動最少」在寫入很貴的儲存裝置上仍有意義。

以下每個函式都用 `less(a, b)` 比較，預設 `std::less<T>`，元素只保證可 move。`items` 是 `std::span<T>`：**原地排序**，不可配置任何記憶體（測試會計算 allocation 次數）。呼叫時寫 `selection_sort<int>(values)`。

## Stage 1 — Selection Sort

`selection_sort<T>(items, less)`：每一輪從尚未排好的部分選出最大的，swap 到正確位置。

- 元素搬動（copy + move）**總數 ≤ 3(n − 1)**，也就是最多 `n − 1` 次 swap（測試用 `Probe` 計數）。
- 比較次數 Θ(n²) 無妨。

## Stage 2 — Insertion Sort

`insertion_sort<T>(items, less)`：把每個元素往前插入已排好的前綴。

- **穩定（stable）**：比較結果相等的元素維持輸入順序。
- **對幾乎排好的資料要快**：令 `I` 為輸入的反序對數量，比較次數 ≤ `2n + I`，搬動次數 ≤ `3(n + I)`。已排序的輸入只需要 O(n)。

## Stage 3 — 實務延伸：Top-K Tracker

`TopK<T, Less>`：持續接收資料，隨時回答「目前最大的 k 個」。儀表板的排行榜、搜尋結果的前十名，k 小的時候直接維護一個排好序的小陣列最實際（k 大時才值得用 heap，Lecture 8 會教）。

- `explicit TopK(k)`：`k ≥ 1`，否則丟 `std::invalid_argument`。
- `push(value)`：O(k)，用 insertion sort 的插入步驟把新元素放進去；超過 `k` 個時淘汰最小的。若新元素不大於目前第 k 名（用 `less` 判斷），就直接丟棄。
- `items()`：`std::span<const T>`，**由大到小**。平手時先進來的排前面（穩定）。
- `size()`：目前保存幾個（`≤ k`）。

每次 `push` 的元素搬動次數不可超過 `k + 8`，不可每次都重新整個排序。

## 面試追問

1. 什麼時候你真的會選 selection sort？寫入成本是比較成本的幾倍時才划算？
2. 為什麼 insertion sort 是 stable，而這份 selection sort 不是？給一個最小的反例。
3. `std::sort` 的 introsort 為什麼在小區間換成 insertion sort？反序對數量跟它的執行時間有什麼關係？
