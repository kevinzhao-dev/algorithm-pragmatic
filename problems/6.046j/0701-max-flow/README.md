---
title: Maximum Flow（Ford-Fulkerson、Edmonds-Karp、Min Cut）
unit: 07 · Network Flow（PS7）
source: 6.046J Lecture 13, Lecture 14
pdf: materials/6.046j/lectures/lec13.pdf
read: materials/6.046j/lectures/lec13.pdf, materials/6.046j/lectures/lec14a.pdf, materials/6.046j/recitations/rec07.pdf
kind: code
minutes: 120
---

# 0701 · Maximum Flow

> **實務上為什麼會遇到**：骨幹網路的可用頻寬、資料中心的 replica 遷移、任何「把量從 A 搬到 B」的容量規劃。瓶頸通常是 min cut：它直接告訴你該升級哪幾條 link，而不是憑感覺加錢。

有向圖，頂點 `0..n-1`，容量為非負整數。允許平行邊和反向邊（`u→v` 與 `v→u` 可同時存在）。`u == v` 的邊不會出現。

一次只看一關。

## Stage 1 — Residual graph 與 Ford-Fulkerson

```cpp
class FlowNetwork {
public:
    explicit FlowNetwork(int n);
    int add_edge(int u, int v, long long capacity);   // 回傳 edge id，從 0 遞增
    long long max_flow_dfs(int s, int t);             // DFS 找 augmenting path
    long long flow_on(int id) const;                  // 最後一次計算之後、該邊的流量
};
```

- 每次呼叫 `max_flow_dfs` 都從零重新計算。
- `s == t` 時丟 `std::invalid_argument`。
- 流量必須滿足 capacity constraint 與 flow conservation；`t` 不可達時回傳 0。

## Stage 2 — Edmonds-Karp

新增 `long long max_flow_bfs(int s, int t)`：每次都找最短（邊數最少）的 augmenting path，O(VE²)。新增 `int augmentations() const`：回傳最後一次計算用了幾條 augmenting path。

## Stage 3 — Min cut 憑證

計算完 max flow 之後：

- `std::vector<bool> min_cut_side() const`：`S` 集合（`s` 在內，`t` 不在內），cut capacity 必須等於 flow 值。
- `std::vector<int> cut_edges() const`：所有 `u∈S, v∉S` 的邊 id，升冪。

## Stage 4 — 實務延伸：把 flow 拆成路徑清單

路由器要的是「這條 LSP 走哪些 link、帶多少量」。`std::vector<PathFlow> decompose() const`（`PathFlow{edges, amount}`）：把目前的 flow 拆成若干條 `s→t` 的 simple path，`edges` 是 edge id 序列，`amount > 0`；amount 總和等於 flow 值，每條邊被分配到的量不超過 `flow_on`，路徑數不超過邊數。

## 面試追問

1. 為什麼 Ford-Fulkerson 在無理數容量下可能不收斂、在整數容量下收斂？Edmonds-Karp 的 augmentation 上界 O(VE) 用了哪個單調性？
2. 把容量大的邊反覆切成小流量，真實系統會遇到什麼問題？Dinic 比 Edmonds-Karp 快在哪？
3. Max-flow min-cut 為什麼可以當成「最佳解證明」？給你一個別人的 max flow 結果，怎麼在 O(E) 內驗證它真的最大？
