---
title: Edge-Disjoint 路由（Menger 憑證與 Hub 限制）
unit: 07 · Network Flow（PS7）
source: 6.046J PS7 Problem 7-2
pdf: materials/6.046j/psets/ps07/questions.pdf
answer: materials/6.046j/psets/ps07/solutions.pdf
read: materials/6.046j/lectures/lec14a.pdf
kind: code
minutes: 110
---

# 0712 · Edge-Disjoint 路由

> **實務上為什麼會遇到**：多租戶資料中心的 failure isolation：k 個 ingest 叢集要把資料送到同一個 sink，任兩條路徑不共用 link，這樣單一 link 故障只影響一個租戶。排不下時，營運要知道瓶頸在哪，而不只是「impossible」。

有向圖 `edges[i] = {u, v}`，頂點 `0..n-1`，可能有平行邊。`sources[0..k-1]` 彼此相異、也不等於 `target`。路徑用 edge index 序列表示：第一條邊的起點是 `sources[i]`，相鄰邊頭尾相接，最後一條邊終點是 `target`，同一條路徑內不重複頂點（simple）。

一次只看一關。

## Stage 1 — k 條 edge-disjoint 路徑

```cpp
std::optional<std::vector<std::vector<int>>> edge_disjoint_routes(
    int n, std::span<const std::pair<int,int>> edges, std::span<const int> sources, int target);
```

回傳 `routes`，其中 `routes[i]` 是 `sources[i]` 到 `target` 的路徑；任兩條路徑不共用 edge index。不可能時回傳 `std::nullopt`。

## Stage 2 — Best-effort 與 Menger 憑證

排不滿時，回傳盡量多的路徑與「為什麼不能更多」的憑證：

```cpp
struct PartialRouting {
    std::vector<std::vector<int>> routes;   // routes[i] 空表示 sources[i] 沒被服務
    std::vector<int> cut_edges;             // edge index，升冪
    std::vector<int> free_sources;          // sources 的索引（0..k-1），升冪
};
PartialRouting best_effort_routes(int n, edges, sources, target);
```

憑證 `(cut_edges, free_sources)` 必須滿足：**不屬於 `free_sources` 的 source**，在移除 `cut_edges` 後到不了 `target`；並且 `routes` 內的路徑數 `== cut_edges.size() + free_sources.size()`。

這正是 Menger 定理：路徑數的上界恰好被一組切割達到。

## Stage 3 — Hub 限制（實務延伸）

某些頂點是中繼 hub，容量有限。`hub_limit[v]` 是**最多幾條路徑可以經過 v**（含作為起點或終點）。`-1` 表示沒有限制。

```cpp
std::optional<std::vector<std::vector<int>>> routes_with_hub_limits(
    int n, edges, sources, target, std::span<const int> hub_limit);
```

路徑依然 edge-disjoint，不可能時回傳 `std::nullopt`。

## 面試追問

1. 為什麼 unit-capacity 的 flow 整數解可以拆成路徑？integrality 的論證在哪裡需要？
2. Hub 限制為什麼要用 node splitting？把容量放到 in/out edge 上為何不夠？
3. 如果 k 條路徑要**點不相交**（vertex-disjoint，含 source/target 不同），演算法怎麼改？
