---
title: Max Subarray 與 LIS（受限子問題）
unit: 07 · Dynamic Programming I（PS7）
source: 6.006 Lecture 16, Recitation 16
pdf: materials/6.006/lectures/lec16.pdf
read: materials/6.006/lectures/lec16.pdf, materials/6.006/recitations/rec16.pdf
kind: code
minutes: 90
---

# 0704 · Max Subarray 與 LIS

> **實務上為什麼會遇到**：Vue 3 重新排列列表時，用 LIS 找出哪些節點可以不動，只移動其他節點，DOM 操作最少。Max subarray 則是「哪一段時間窗的異常最嚴重」這種事後分析。兩題的關鍵一樣：光看 prefix 不夠，子問題要多綁一個條件「必須以第 k 個元素結尾」。

一次只看一關，做完再往下讀。

## Stage 1 — Max Subarray

```cpp
struct Subarray { std::int64_t sum; std::size_t first; std::size_t last; };  // inclusive
Subarray max_subarray(std::span<const std::int64_t> values);
```

回傳總和最大的非空連續區段，O(n) 時間。空輸入丟 `std::invalid_argument`。全部為負時就是其中最大的單一元素。同分時回傳任一段即可。

## Stage 2 — LIS 長度

`lis_length(values)`：`values` 是 `std::ranges::random_access_range`，元素只需要 `operator<`，回傳**嚴格遞增**子序列的最長長度。相等的元素不算遞增。這一關 O(n²) 就夠了。

## Stage 3 — 還原一組 LIS

`std::vector<std::size_t> lis_indices(values)`：回傳一組最長嚴格遞增子序列的索引，由小到大。同長度時任一組都可以。

## Stage 4 — O(n log n)

`lis_length` 和 `lis_indices` 都要改成最多約 `n log₂ n` 次 `operator<`（測試用 `Counted` 計數，不靠時鐘）。

提示：維護「長度為 k 的遞增子序列，最小的結尾是什麼」，這張表是遞增的，所以可以二分搜尋。結尾只有一個數字不夠還原解，每個元素還要記下自己的前一個是誰。

## Stage 5 — 實務延伸：最少移動的重新排列

```cpp
template <std::ranges::random_access_range R>
std::vector<std::size_t> keep_in_place(const R& before, const R& after);
```

`before` 和 `after` 是同一組不重複 key 的兩種排列，把 `before` 變成 `after` 的一次操作是「拿起一個元素、插到任意位置」。回傳 `after` 中**不需要移動**的元素索引（由小到大），數量要盡量多，於是移動次數 `n - keep` 最少。Key 需可雜湊。

## 面試追問

1. 為什麼子問題「prefix 的 LIS 長度」做不出 recurrence？綁定「以 k 結尾」解決了什麼？
2. Stage 4 的表**不是**一組 LIS 本身，為什麼長度卻是對的？
3. Stage 5 的移動次數為什麼不可能比 `n - LIS` 更少？如果移動成本隨距離增加，還能用 LIS 嗎？
