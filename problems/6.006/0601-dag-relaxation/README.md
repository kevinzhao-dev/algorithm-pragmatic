---
title: DAG Relaxation、最短路徑樹與 Critical Path
unit: 06 · Weighted Shortest Paths（PS6）
source: 6.006 Lecture 11, Recitation 11
pdf: materials/6.006/lectures/lec11.pdf
read: materials/6.006/recitations/rec11.pdf
kind: code
minutes: 100
---

# 0601 · DAG Relaxation、最短路徑樹與 Critical Path

> **實務上為什麼會遇到**：CI pipeline 的總時長、專案排程的 critical path、資料管線的端到端延遲，算的都是 DAG 上的最長或最短路徑。只要圖沒有環，就不需要 Bellman-Ford 或 Dijkstra。

頂點編號 `0 … n-1`。起點不存在時丟 `std::out_of_range`，後面每關都一樣。一次只看一關，做完再往下讀。

```cpp
struct Arc { int to; std::int64_t weight; };
using WeightedGraph = std::vector<std::vector<Arc>>;  // weights may be negative
inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr int kNoParent = -1;
```

## Stage 1 — DAG 上的最短路徑

`dag_shortest_paths(g, s)` 回傳 `dist`：`dist[v]` 是 `s` 到 `v` 的最短路徑權重，到不了是 `kUnreachable`。權重可以是負的。圖裡有 directed cycle 時丟 `std::invalid_argument`（不管 cycle 在哪裡）。

要 O(V + E)，頂點可能有上百萬個。

## Stage 2 — 從距離還原最短路徑樹

`shortest_path_tree(g, s, dist)`：已知 `dist` 是**任意**圖從 `s` 出發的真實最短距離（圖不一定是 DAG，保證沒有從 `s` 可達的負環）。回傳 `parent`：

- 對每個可達的 `v != s`，邊 `parent[v] → v` 必須存在且滿足 `dist[parent[v]] + w == dist[v]`，而且沿著 `parent` 一路往回走一定會回到 `s`。
- `s` 和到不了的頂點是 `kNoParent`。

要 O(V + E)。圖裡可能有權重總和為 0 的環。

## Stage 3 — 實務延伸：CI Pipeline

```cpp
struct Job {
    std::int64_t duration;        // >= 0
    std::vector<int> depends_on;  // jobs that must finish before this one starts
};
struct PipelinePlan {
    std::int64_t makespan;
    std::vector<std::int64_t> earliest_start;
    std::vector<std::int64_t> slack;  // how long a job can slip without delaying the pipeline
    std::vector<int> critical_path;   // one longest chain of jobs, in execution order
};
```

機器數量沒有限制，能開始的工作會立刻開始。`plan_pipeline(jobs)` 回傳：

- `earliest_start[i]`：工作 `i` 最早的開始時間；`makespan` 是所有工作都做完的時間。
- `slack[i]`：工作 `i` 最多可以延後多久，整條 pipeline 仍然在 `makespan` 內完成。
- `critical_path`：一條前後有相依關係、首尾相接的工作鏈，第一個工作從時間 0 開始，最後一個工作在 `makespan` 結束。

沒有工作時全部是空的、`makespan` 為 0。相依成環丟 `std::invalid_argument`，`depends_on` 裡有不存在的工作丟 `std::out_of_range`。

要 O(V + E)。

## 面試追問

1. DAG Relaxation 為什麼只要依拓撲順序把每條邊 relax 一次就夠？用歸納法講給面試官聽。
2. Stage 2 為什麼不能「每個頂點隨便挑一條滿足等式的進入邊」？零權重環會造成什麼？
3. Longest path 在一般圖上是 NP-hard，為什麼在 DAG 上卻這麼容易？`slack` 為 0 的工作就一定在 critical path 上嗎？
