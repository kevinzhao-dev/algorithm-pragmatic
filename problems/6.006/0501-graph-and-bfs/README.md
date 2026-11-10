---
title: Graph 表示法與 BFS
unit: 05 · Graph Search（PS5）
source: 6.006 Lecture 9, Recitation 9
pdf: materials/6.006/lectures/lec09.pdf
read: materials/6.006/recitations/rec09.pdf
kind: code
minutes: 90
---

# 0501 · Graph 表示法與 BFS

> **實務上為什麼會遇到**：服務相依圖的影響範圍、社群網路的幾度分隔、爬蟲的抓取順序，都是 unweighted graph 上的 BFS。後面的題目會一直把它搬到更抽象的圖上跑，所以這裡要先寫到不用想。

頂點編號固定是 `0 … n-1`。一次只看一關，做完再往下讀。

## Stage 1 — Adjacency list

實作 `Graph`。

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `Graph(n)` | `n` 個頂點、沒有邊；`n < 0` 丟 `std::invalid_argument` | O(n) |
| `vertex_count()` / `edge_count()` | `add_*` 每呼叫一次算一條邊 | O(1) |
| `add_directed(u, v)` | 加入 `u → v` | 平均 O(1) |
| `add_undirected(a, b)` | 兩端的 adjacency list 各放一筆；`a == b` 時只放一筆 | 平均 O(1) |
| `neighbors(u)` | 依加入順序回傳 `std::span<const int>` | O(1) |
| `out_degree(u)` / `in_degree(v)` | adjacency list 裡的筆數 / 指向 `v` 的筆數 | O(1) |

不去重：平行邊和 self-loop 都照存。任何函式拿到不存在的頂點都丟 `std::out_of_range`（後面幾關也一樣）。

## Stage 2 — BFS 與最短路徑樹

- `bfs(g, s)` 回傳 `{dist, parent}`。`dist[v]` 是 `s` 到 `v` 的最少邊數，到不了是 `-1`；`parent[v]` 是某條最短路徑上 `v` 的前一個頂點，`s` 和到不了的頂點是 `kNoVertex`。
- `shortest_path(g, s, t)`：回傳從 `s` 到 `t` 的頂點序列（含兩端），到不了回傳 `std::nullopt`。
- `level_sets(g, s)`：`result[i]` 是距離恰為 `i` 的所有頂點，內部順序不拘，最後不留空的 level。

都要 O(V + E)。

## Stage 3 — 奇數步路徑

`shortest_odd_walk(g, s, t)`：回傳從 `s` 到 `t`、**邊數為奇數**的最短 walk（可以重複經過頂點，`s == t` 時至少要走一步），不存在回傳 `std::nullopt`。

要 O(V + E)。

## Stage 4 — 實務延伸：幾度分隔

`separation(g, a, b)`：`g` 是 undirected graph，回傳 `a` 到 `b` 的最少邊數，不連通回傳 `std::nullopt`。

單向 BFS 在這裡太慢：圖有一百萬個頂點，但兩個使用者通常只隔幾層。請從兩端同時往外搜，frontier 一碰到就停，成本要跟兩邊實際走過的範圍成正比，而不是整張圖。

## 面試追問

1. 邊數接近 V² 時，adjacency matrix 和 adjacency list 各自贏在哪裡？`neighbors` 回傳的 `span` 在什麼時候會失效？
2. BFS 為什麼能保證距離最短？如果邊有 0 和 1 兩種權重，要怎麼改？
3. 雙向 BFS 的 frontier 第一次相遇時，答案一定是最短嗎？你的實作靠什麼保證？最壞情況會比單向 BFS 好嗎？
