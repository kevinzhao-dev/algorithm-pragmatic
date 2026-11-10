---
title: 遞迴 Tableau 的快取複雜度（k²-way 的運氣問題）
unit: 11 · Caching（HW8）
source: 6.172 HW8 §2.2 Write-up 4–5
pdf: materials/6.172/homework/hw08/handout.pdf
read: materials/6.172/lectures/lec14.pdf, materials/6.172/lectures/lec15.pdf
kind: code
minutes: 70
---

# 1114 · 遞迴 Tableau 的快取複雜度

> **實務上為什麼會遇到**：cache-oblivious 的保證是漸近的，切法會影響「有多接近」。k 取太大時，若某一層的小塊只比快取大一點點，常數會差很多；k = 2 則沒有這個問題。這個現象在調 B-tree fanout 或 chunk 大小時會一再遇到。

延續 1112、1113 的壓縮陣列與 `f`，這題換成用模擬器（`_shared/08_cache_sim.hpp`，全部用 fully-associative）量 miss。

## Stage 1 — 遞迴 tableau

`recursive_tableau(a, n, init, f)`：和 1113 Stage 1 相同的四象限遞迴（左上、右上、左下、右下，`mid = begin + len/2`，範圍是第 1..n−1 列、行），`f(A(i-1,j-1), A(i,j-1), A(i-1,j))`，回傳 `A(n-1, n-1)`。

## Stage 2 — Q(n)（Write-up 4）

`predict_recursive_misses(n, cache)`：回傳遞迴版本的 Θ 估計（`CacheGeometry`：M、B，單位都是元素）。提示：遞迴切到一塊可以放進快取時，這塊只要 `Θ(M/B)` 個 miss，葉子有 `Θ((n/M)²)` 塊。實際 miss 數會落在估計的 0.5 到 12 倍之間。

## Stage 3 — k²-way 與運氣（Write-up 5）

`recursive_tableau_k(a, n, k, init, f)`：把範圍每一維切成 k 份（切點 `lo + len * p / k`，p = 0..k，空的片段略過），k² 個小塊的順序是列由小到大、同列內行由小到大。base case 是 1×1 的 cell。

Write-up 5 說：如果某一層的小塊只比快取大一點，就會「不幸運」：`Θ(n²k/(MB))`，比幸運時的 `Θ(n²/(MB))` 多一個 k 倍。測試會在 k = 16 下，比較 n 恰好不幸運與幸運時的 `Q(n)/n²`，並確認 k = 2 沒有這個現象。

## 面試追問

1. Write-up 4 的完整推導：Q(n) 的 recurrence、遞迴樹高、葉子數量、加總時哪一項主導？
2. Write-up 5 的 unlucky 情況：只比快取大一點的小塊，為什麼 k 越大、裡面的小 piece 越小，miss 反而變多？
3. Lecture 15 的 merge sort 也有類似的取捨：k-way merge 要怎麼選 k？cache-oblivious 的 funnel sort 用什麼手法避開這個參數？
