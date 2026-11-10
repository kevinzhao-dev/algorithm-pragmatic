---
title: 距離限制的兩數和（close pair）
unit: 03 · Hashing & Linear Sorting（PS3）
source: 6.006 PS3 Problem 3-4
pdf: materials/6.006/psets/ps3/questions.pdf
answer: materials/6.006/psets/ps3/solutions.pdf
read: materials/6.006/lectures/lec04.pdf, materials/6.006/lectures/lec05.pdf
kind: code
minutes: 60
---

# 0314 · 距離限制的兩數和

> **實務上為什麼會遇到**：資料壓實（compaction）要從相鄰的 segment 找出總大小剛好等於目標 block 的組合；撮合引擎要在最近幾筆委託裡找互抵的一對。「兩數和」加上位置限制，是 hash 和排序的經典分界點。

給定長度 `n` 的陣列 `b`（元素是**相異的正整數**）和目標 `r`。一對索引 `(i, j)`，`i < j`，若 `b[i] + b[j] == r` 且 `|i − j| < n/10`（是實數除法，`10·(j−i) < n`，不是整數除法的 `n/10`），就是一個 **close pair**。

兩個函式都回傳任一個 close pair（以 `(i, j)`，`i < j` 表示），不存在時回傳 `std::nullopt`。

## Stage 1 — 期望 O(n)

`close_pair_expected(b, r)`：期望 O(n)，可以用 hash table。

陷阱：

- `r - b[i]` 在 `uint64_t` 上可能 underflow；`b[i] + b[j]` 也可能溢位（`b` 的值可能接近 `2^64`）。先想清楚再相減。
- `b[i] == r/2` 時不能跟自己配對。
- `n = 10` 時最小距離 1 不合法（`1 < 1` 不成立）；`n = 11` 時距離 1 合法（`1 < 1.1`）。

## Stage 2 — 最差 O(n)

`close_pair_worst_case(b, r)`：保證 `r < n²`，要**最差情況** O(n)，**不可使用 hash table**。

先想：`b` 的值可能遠大於 `n²`（例如 `2^60`），為什麼這不會把 radix sort 變慢？排好之後用雙指標（two-finger）掃一次；記得每個值最多只有一個配對對象。可以 `#include "../0303-counting-radix-sort/solution.hpp"`，重用你的 `radix_sort`。

## Stage 3 — 實務延伸：串流視窗配對

`WindowPairMatcher(window, target)`，`window ≥ 1`（否則丟 `std::invalid_argument`）：

- `offer(value)`：第 `i` 次呼叫（從 0 起算）代表資料位置 `i`。回傳 `[i−window, i−1]` 範圍內**最近**的位置 `j`，使 `value_j + value_i == target`；沒有時回傳 `std::nullopt`。這裡的值**可能重複**，也可能是 0。
- `tracked()`：內部還記得的相異 value 數；過了視窗的舊資料要忘掉，上限是 `window`。

每次 `offer` 要期望 O(1)。

## 面試追問

1. Stage 2 的「把 `≥ r` 的值丟掉」為什麼是 radix sort 能線性的關鍵？如果是 `r < n^100` 呢？
2. Stage 2 的雙指標，當 `sum == r` 但距離太遠時，為什麼可以直接把左指標往右？
3. Stage 3 如果 key 不能離開記憶體（比如 key 是 TB 級），hash map 還是對的工具嗎？
