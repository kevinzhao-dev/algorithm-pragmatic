---
title: FPT：Vertex Cover 的 Bounded Search 與 Kernel
unit: 09 · Approximation 與 FPT（PS9）
source: 6.046J Lecture 18
pdf: materials/6.046j/lectures/lec18.pdf
read: materials/6.046j/lectures/lec18.pdf
kind: code
minutes: 120
---

# 0902 · FPT：Vertex Cover

> **實務上為什麼會遇到**：資料去重、conflict-graph 的修復，通常 k 很小但 n 上千萬。把指數部分隔離到參數 k，就能在 n 很大的時候精確求解——`2^k · n` 跟 `n^k` 是完全不同的世界。

無向圖 `edges`，頂點 `0..n-1`，**可能有重複邊**（輸入保證無 self-loop）。

一次只看一關。

## Stage 1 — Bounded Search Tree

`std::optional<std::vector<int>> vc_search(int n, std::span<const std::pair<int,int>> edges, int k)`：若存在大小 ≤ `k` 的 vertex cover，回傳一個（頂點不重複，順序不限）；否則回傳 `std::nullopt`。時間 O(2^k·(V+E))，深度不可超過 k。

講義的做法：挑一條還沒被覆蓋的邊 `(u,v)`，分別試 `u` 在 cover 裡、`v` 在 cover 裡。

## Stage 2 — Kernelization

```cpp
struct Kernel {
    bool no_instance = false;
    int n = 0;
    std::vector<std::pair<int,int>> edges;   // simple, n vertices (no isolated ones)
    int k = 0;
    std::vector<int> forced;                 // original ids, must be in any size-≤k cover
    std::vector<int> to_original;            // to_original[i] = original id of kernel vertex i
};
Kernel kernelize(int n, std::span<const std::pair<int,int>> edges, int k);
```

規則（重複套用）：刪除重複邊；度數 `> k` 的頂點必定在 cover 裡，加入 `forced`、刪除、`k-1`（`k` 為負 ⇒ `no_instance`）。之後若邊數 `> k²` ⇒ `no_instance`；否則刪除孤立點，並把剩下的頂點重新編號。需滿足：`n ≤ 2k²`、邊數 ≤ `k²`。

## Stage 3 — Kernel + 搜尋

`std::optional<std::vector<int>> vc_fpt(int n, edges, int k)`：先 kernelize 再 bounded search，總時間 O(V + E + 2^k·k²)。回傳原始編號的 cover，大小 ≤ k。

## Stage 4 — 實務延伸：最小 cover

`std::vector<int> min_vertex_cover(int n, edges)`：回傳一個最小 vertex cover。k 從 lower bound 往上試（iterative deepening），OPT 很小時整體依然很快。

## 面試追問

1. 為什麼 `f(k)·n^{O(1)}` 算好、`n^{f(k)}` 不算？Kernelization 的定義和 FPT 為什麼等價？
2. 「度數 > k 的頂點必選」這條規則的正確性是什麼？為什麼邊數 > k² 就可以直接拒絕？
3. 這題的 `2^k` 能改進到 `1.47^k` 嗎？（對度數 ≥ 3 的頂點分岔）
