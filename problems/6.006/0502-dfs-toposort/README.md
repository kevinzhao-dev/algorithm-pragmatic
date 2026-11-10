---
title: DFS、拓撲排序與環偵測
unit: 05 · Graph Search（PS5）
source: 6.006 Lecture 10, Recitation 10
pdf: materials/6.006/lectures/lec10.pdf
read: materials/6.006/recitations/rec10.pdf
kind: code
minutes: 100
---

# 0502 · DFS、拓撲排序與環偵測

> **實務上為什麼會遇到**：build system、資料管線、migration 順序，全都要先排出拓撲順序；排不出來時，錯誤訊息還得說清楚是哪幾個東西繞成一圈。

圖用 `AdjacencyList`（`std::vector<std::vector<int>>`）表示，`g[u]` 是 `u` 的 outgoing neighbors，頂點編號 `0 … n-1`。一次只看一關，做完再往下讀。

**全部不可以遞迴**：圖可以是百萬個頂點的一條長鏈。起點不存在時丟 `std::out_of_range`。

## Stage 1 — DFS

`dfs(g, s)` 回傳 `DfsForest{parent, finish_order}`：

- 依 `g[u]` 的順序走鄰居，第一次碰到的頂點就往下走。**結果要和教科書的遞迴版完全相同**。
- `parent[v]` 是發現 `v` 的頂點；`s` 和沒走到的頂點是 `kNoParent`。
- `finish_order` 是 `s` 可達的頂點，依「visit 結束」的先後排列。

O(V + E)。

## Stage 2 — Full-DFS 與連通元件

- `full_dfs(g)`：把所有頂點走完，每個還沒走到的頂點依編號由小到大當新的 root，root 的 `parent` 是 `kNoParent`。
- `connected_components(g)`：`g` 是 undirected（`v` 在 `g[u]` 裡則 `u` 也在 `g[v]` 裡）。回傳每個頂點的元件編號，編號依元件中最小頂點出現的先後從 0 開始。

## Stage 3 — 拓撲排序，或交出環

`topological_sort(g)` 回傳 `std::variant<TopologicalOrder, DirectedCycle>`：

- 沒有環：`TopologicalOrder.vertices` 是所有頂點的一個排列，每條邊的 tail 都排在 head 前面。
- 有環：`DirectedCycle.vertices` 是一個**簡單**環，`vertices[i] → vertices[i+1]` 都是圖上的邊，最後一個頂點接回第一個。self-loop 是長度為 1 的環。

O(V + E)。

## Stage 4 — 實務延伸：增量 build

做一個 `BuildGraph`，`edges` 裡的 `{a, b}` 表示 `a` 要先於 `b` 建置：

- `BuildGraph(targets, edges)`：邊有環時丟 `CycleError`，它的 `cycle()` 回傳一個同 Stage 3 格式的環；`edges` 裡有不存在的 target 時丟 `std::out_of_range`。
- `rebuild_order(changed)`：`changed` 裡的 target 改了之後，要重建的就是它們加上它們所有的下游。回傳這些 target 的一個合法建置順序，每個只出現一次。`changed` 可以有重複，也可以是空的；有不存在的 target 時丟 `std::out_of_range`。

專案有兩百萬個 target，開發者一天改幾百次檔案：**每次查詢的成本只能跟受影響的子圖成正比**，不能每次都掃過整張圖。

## 面試追問

1. 為什麼反向的 finishing order 是拓撲順序？證明要分哪兩個 case？
2. Undirected graph 的 DFS 為什麼不會出現 forward edge 和 cross edge？
3. 跟 Kahn's algorithm（反覆拿掉 in-degree 為 0 的頂點）比，DFS 版有什麼好處、壞處？如果 `rebuild_order` 要讓多個執行緒同時呼叫，你用來記錄 visited 的東西要放在哪裡？
