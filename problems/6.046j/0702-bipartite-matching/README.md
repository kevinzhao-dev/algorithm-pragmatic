---
title: Bipartite Matching（König 定理與 Hall 憑證）
unit: 07 · Network Flow（PS7）
source: 6.046J Lecture 14, Recitation 7
pdf: materials/6.046j/lectures/lec14a.pdf
read: materials/6.046j/lectures/lec14a.pdf, materials/6.046j/recitations/rec07.pdf
kind: code
minutes: 110
---

# 0702 · Bipartite Matching

> **實務上為什麼會遇到**：工作分派給機器、廣告版位配對、Pod 排到節點。當排不滿的時候，真實系統要回答「為什麼排不滿」，這就是 Hall 違反集合的用途。

左側頂點 `0..left-1`，右側 `0..right-1`，邊 `{l, r}`。`std::span<const std::pair<int,int>>` 輸入，可能有重複邊。

一次只看一關。

## Stage 1 — 最大配對

```cpp
struct Matching {
    int size = 0;
    std::vector<int> match_left;   // match_left[l]  = 配到的右側頂點，或 -1
    std::vector<int> match_right;  // match_right[r] = 配到的左側頂點，或 -1
};
Matching max_matching(int left, int right, std::span<const std::pair<int,int>> edges);
```

O(VE) 即可（augmenting path 或 flow 都行）。

## Stage 2 — König：最小點覆蓋

Recitation 7 把二分圖點覆蓋化成 min cut。實作 `VertexCover min_vertex_cover(int left, int right, edges)`，回傳 `VertexCover{left, right}`：左右被選到的頂點（升冪），覆蓋每條邊，總大小最小。

König 定理保證大小等於最大配對。

## Stage 3 — Hopcroft-Karp

`Matching hopcroft_karp(int left, int right, edges)`：O(E√V)。每個 phase 先 BFS 分層，再用 DFS 沿分層找一組極大 vertex-disjoint augmenting paths。

## Stage 4 — 實務延伸：Hall 違反集合

`std::vector<int> hall_violator(int left, int right, edges)`：若存在讓所有左側頂點都配對的 matching，回傳空 vector；否則回傳一個左側子集 `S`（非空、升冪），滿足 `|N(S)| < |S|`（鄰居數少於自身數）。

## 面試追問

1. 從 flow network 的 min cut 怎麼讀出 König 的 vertex cover？（`S ∩ R` 和 `T ∩ L` 為什麼對？）
2. Hopcroft-Karp 為什麼只要 O(√V) 個 phase？
3. 如果要的是「權重最大」的 matching，這些方法哪些還能用？（Hungarian / min-cost flow）
