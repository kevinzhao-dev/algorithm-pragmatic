---
title: Merge Sort 與反序數
unit: 02 · Sorting & Recurrences（PS2）
source: 6.006 Lecture 3, Recitation 3
pdf: materials/6.006/lectures/lec03.pdf
read: materials/6.006/recitations/rec03.pdf
kind: code
minutes: 70
---

# 0203 · Merge Sort

> **實務上為什麼會遇到**：`std::stable_sort`、外部排序（資料放不進記憶體）、LSM tree 的 compaction 都是 merge；而「兩個排名差多少」用反序數量化（Kendall tau），推薦系統的評估天天在用。

所有比較都用 `less(a, b)`，預設 `std::less<T>`。

## Stage 1 — Merge

`merge_sorted<T>(a, b, less)`：`a`、`b` 都已排序（`std::span<const T>`），回傳合併後排序好的 `std::vector<T>`。

- 比較次數 ≤ `|a| + |b|`。
- **穩定**：比較相等時，`a` 的元素排在 `b` 的元素之前。

## Stage 2 — Merge Sort

`merge_sort<T>(items, less)`：把 `items`（`std::span<T>`）原地排序完成（可以用額外空間，但呼叫結束後 `items` 是排好的）。元素只保證可 move，不要求 default-constructible。

- **穩定**。
- 比較次數 ≤ `1.5 · n · ⌈log₂ n⌉ + n`（兩兩合併的最壞情況；測試用計數檢查）。
- **整個排序只配置一次暫存空間**（heap allocation 次數 ≤ 2，由測試計算）：不要每次遞迴都建新的 `vector`。

提示：一個長度 `n` 的 buffer 就夠。寫個輔助函式把 `items[lo..hi)` 排好，merge 時把左半搬進 buffer，再從 buffer 和右半合併回去（或直接合併到 buffer 再搬回）。

## Stage 3 — 實務延伸：排名距離

- `count_inversions(values)`：`values` 中滿足 `i < j` 且 `values[i] > values[j]` 的 pair 數量，**O(n log n)**，不可以暴力 O(n²)。答案用 `uint64_t`。
- `kendall_tau_distance(rank_a, rank_b)`：`rank_a`、`rank_b` 都是 `0…n−1` 的排列，代表兩種排名（例如新舊兩版搜尋演算法對同一批結果的排序）。回傳「被兩種排名排序方向不同的 pair 數量」。長度不同或不是排列時丟 `std::invalid_argument`。

提示：merge 時，每當右半的元素比左半剩下的元素先出列，就累加 inversions。

## 面試追問

1. 為什麼 merge sort 的遞迴深度是 O(log n)，但 stack 用量不是問題？真正的額外空間在哪？能做到 in-place merge 嗎，代價是什麼？
2. 寫出 merge sort 的 recurrence，並說明 Master Theorem 的哪個 case 適用。
3. 反序數跟 insertion sort 的 swap 次數有什麼關係（對照 0202 和 0212）？
