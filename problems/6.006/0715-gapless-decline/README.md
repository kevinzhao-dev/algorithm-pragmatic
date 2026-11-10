---
title: 不跳時段的最長下降趨勢（受限 LIS）
unit: 07 · Dynamic Programming I（PS7）
source: 6.006 PS7 Problem 7-5
pdf: materials/6.006/psets/ps7/questions.pdf
answer: materials/6.006/psets/ps7/solutions.pdf
read: materials/6.006/lectures/lec16.pdf, materials/6.006/recitations/rec16.pdf
kind: code
minutes: 110
---

# 0715 · 不跳時段的最長下降趨勢

> **實務上為什麼會遇到**：監控系統想找「指標持續惡化、中間沒有恢復的時段」來判定事故的開始。每個時間 bucket 有多個樣本，要的不是全域的 LIS，而是連續幾個 bucket 內的趨勢。真實資料的 `k` 很大，O(k²) 的轉移撐不住，於是這題要在 DP 上疊資料結構。

一條 series 有 `n` 個連續的時間 bucket，每個 bucket 恰好 `k` 個樣本，平攤成長度 `n·k` 的陣列，前 `k` 個屬於 bucket 0，依此類推。

合法的**趨勢**是嚴格遞減的子序列，而且不跳 bucket：若它含有 bucket `a` 和 `b`（`a < b`）的樣本，那 `a..b` 之間每個 bucket 都至少要有一個樣本。

一次只看一關，做完再往下讀。

## Stage 1 — 長度

`std::size_t gapless_decline_length(std::span<const int> samples, std::size_t per_bucket)`

- 回傳最長趨勢的長度，O(n·k²) 時間就夠了（`samples.size() == n·k`）。
- `per_bucket == 0` 或 `samples.size()` 不是 `per_bucket` 的倍數時丟 `std::invalid_argument`。
- 空的 `samples` 回傳 0。

下一個樣本可以在同一個 bucket 較後面的位置，也可以在下一個 bucket。

## Stage 2 — 還原一條趨勢

`std::vector<std::size_t> gapless_decline(std::span<const int> samples, std::size_t per_bucket)`：回傳一條最長趨勢的索引（遞增）。同長度任選一條。

## Stage 3 — 多條 series 中最差的一條

```cpp
struct WorstSeries { std::size_t series; std::vector<std::size_t> indices; };
WorstSeries worst_series(std::span<const std::vector<int>> series, std::size_t per_bucket);
```

回傳趨勢最長的那條 series 的索引，以及一條最長趨勢。同長度時任選。`series` 為空或任一條長度不是 `per_bucket` 的倍數時丟 `std::invalid_argument`。

## Stage 4 — O(n·k·log k)

`gapless_decline_length`、`gapless_decline`、`worst_series` 都要改到 O(n·k·log k)（`worst_series` 再乘上 series 數）。

Stage 1 的轉移要在一個區間內找「值比我小、又最長」的候選。這個區間由兩部分組成：同 bucket 後面的樣本、下一個 bucket 的全部樣本。下一個 bucket 是固定的集合，同 bucket 則隨著掃描逐漸長大；兩者要用不同的方法維護。

## 面試追問

1. 為什麼「不跳 bucket」讓 LIS 的 patience 演算法（0704 Stage 4）不能直接用？
2. Stage 4 的資料結構需要支援什麼操作？如果區間是「前後 `h` 個 bucket」的通用滑動窗，你會怎麼處理刪除？
3. `n·k` 已經是輸入大小，為什麼這題是 polynomial 而不是 pseudopolynomial？
