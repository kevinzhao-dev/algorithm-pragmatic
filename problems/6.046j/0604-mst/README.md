---
title: Minimum Spanning Tree（Kruskal、Prim、clustering）
unit: 06 · Augmentation、APSP 與 MST（PS6）
source: 6.046J Lecture 12
pdf: materials/6.046j/lectures/lec12.pdf
read: materials/6.046j/lectures/lec12.pdf
kind: code
minutes: 80
---

# 0604 · Minimum Spanning Tree

> **實務上為什麼會遇到**：資料中心 cabling 成本、影像分割、single-linkage clustering，以及把「在分散節點之間建最便宜的連通骨幹」這類需求，都是 MST。

無向圖，邊 `WeightedEdge{u, v, w}`，權重可重複。回傳的是輸入 `edges` 的索引。圖可以不連通，此時算的是 minimum spanning **forest**。

Union-Find 請沿用 0402 的成果（或重寫一次），不會單獨測試。一次只看一關。

## Stage 1 — Kruskal

`SpanningForest kruskal(int n, std::span<const WeightedEdge> edges)`，其中 `SpanningForest{total, edges}`。O(E log E)。

邊界：沒有邊、平行邊、不連通。

## Stage 2 — Prim

`SpanningForest prim(int n, std::span<const WeightedEdge> edges)`：binary heap 版本，O(E log V)。不連通時要從每個未走訪的連通元件各跑一次。結果的 `total` 必須等於 Kruskal。

## Stage 3 — 實務延伸：single-linkage clustering

`std::vector<int> cluster(int n, std::span<const WeightedEdge> edges, int k)`：把頂點分成恰好 `k` 群（`1 ≤ k ≤ n`，輸入保證連通），讓群與群之間的最小邊盡可能大（maximum spacing）。回傳每個頂點的群編號 `0..k-1`，編號依各群最小頂點編號遞增排列。

## 面試追問

1. Cut property 和 cycle property 分別保證了什麼？Prim 和 Kruskal 各靠哪一個？
2. 權重有重複時，MST 還唯一嗎？如何在 Kruskal 的過程中判斷？
3. Prim 用 binary heap 和 Fibonacci heap，在稠密圖上差多少？實務上為什麼很少用後者？
