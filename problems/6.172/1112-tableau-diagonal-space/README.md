---
title: Tableau 的 2N−1 空間壓縮與快取分析
unit: 11 · Caching（HW8）
source: 6.172 HW8 §2.1 Write-up 1–2（iterative tableau）
pdf: materials/6.172/homework/hw08/handout.pdf
read: materials/6.172/lectures/lec08.pdf, materials/6.172/lectures/lec14.pdf
kind: code
minutes: 60
---

# 1112 · Tableau 的 2N−1 空間壓縮與快取分析

> **實務上為什麼會遇到**：DP 表格（LCS、edit distance、sequence alignment）只需要最後一格時，空間可以大幅壓縮。這題的壓縮方式剛好讓「每一格的鄰居」都還沒被覆蓋掉，也讓快取行為很容易推導，是很多 stencil 計算的縮影。

N×N 的 tableau 只需要 `A(N−1, N−1)`，所以只用一個長度 `2N − 1` 的陣列 `a`：格子 `(i, j)` 放在索引 `N + i − j − 1`（每條對角線一格）。依列填表時，算 `(i, j)` 要用到的 `(i−1, j−1)`、`(i, j−1)`、`(i−1, j)` 都還在正確的位置上（想想為什麼）。

## Stage 1 — 填表（Write-up 1）

- `diagonal_index(i, j, n)`：回傳上面的對應索引。
- `fill_tableau(a, n, init, f)`：`a` 恰好有 `2n − 1` 格（介面只有 `size()`、`get(k)`、`set(k, v)`），不可以配置額外的陣列。先把第一列和第一行都設成 `init`，然後依列由上到下、每列由左到右計算其餘的格子，`f(A(i−1,j−1), A(i,j−1), A(i−1,j))`，回傳 `A(n−1, n−1)`。`n == 1` 時直接回傳 `init`。

## Stage 2 — 快取分析（Write-up 2）

`predict_iterative_misses(n, cache)`：回傳 `fill_tableau` 的 Θ 估計（`cache` 同 1111，都以元素為單位）。先自己推，再來跟模擬器對。分成兩種情況：

- `n` 比快取小：快取夠放一整列的工作範圍，會發生什麼事？
- `n` 比快取大：每一列掃過的 line 都要重載一次嗎？

測試（M = 512 elements，B = 8 elements，fully associative）會比較預測與實際，兩邊要差不超過 2 倍，也會驗證 miss 數隨 n 成長的趨勢。

## 面試追問

1. 為什麼這個對應方式恰好讓空間夠用？如果把 `A(i, j)` 放到索引 `i + j`（反對角線）會發生什麼事？（作業 Write-up 1）
2. 你的分析裡用到 cache 需要多大才放得下一列？tall cache 的假設有用到嗎？
3. 把 Lecture 8 的 tableau 平行化時，這個壓縮陣列還能用嗎？（見 1113）
