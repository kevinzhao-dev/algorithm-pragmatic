---
title: Graph 手算題轉成程式（表示法、BFS/DFS 軌跡、單邊解環）
unit: 05 · Graph Search（PS5）
source: 6.006 PS5 Problem 5-1
pdf: materials/6.006/psets/ps5/questions.pdf
answer: materials/6.006/psets/ps5/solutions.pdf
read: materials/6.006/lectures/lec09.pdf, materials/6.006/lectures/lec10.pdf
kind: code
minutes: 60
---

# 0511 · Graph 手算題轉成程式

> **實務上為什麼會遇到**：看懂一份 BFS/DFS 的走訪軌跡是除錯 graph 程式的基本功；「拿掉哪一條依賴就能解開循環」則是整理 monorepo 時真的會被問的問題。

原題是手算，這裡把每一小題都寫成程式。圖都是 `0 … n-1` 的 simple directed graph，沒有 self-loop、沒有平行邊。一次只看一關，做完再往下讀。

## Stage 1 — 表示法互轉

- `to_adjacency_list(matrix)`：`matrix[u][v] == 1` 代表有邊 `u → v`；每條 adjacency list 由小到大排好。不是 `n × n` 或有 0、1 以外的值時丟 `std::invalid_argument`。
- `to_adjacency_matrix(list)`：反過來。鄰居超出範圍或重複時丟 `std::invalid_argument`。

## Stage 2 — 走訪軌跡

兩個函式都回傳 `SearchTrace{visit_order, parent}`：`visit_order` 是第一次拜訪到的先後，`parent` 是每個頂點在搜尋樹裡的父節點（起點和沒拜訪到的是 `kNoParent`）。

- `bfs_trace(g, s)`、`dfs_trace(g, s)`：**鄰居一律由小到大處理**，輸入的 adjacency list 不保證已排序。`dfs_trace` 要和遞迴版完全一致（往下走到底才回頭）。

起點不存在時丟 `std::out_of_range`。

## Stage 3 — 單邊解環

`single_edge_fixes(g)`：找出**所有**「拿掉這一條邊，整張圖就變成 DAG」的邊。回傳 `std::vector<EdgeFix>`，依 `(from, to)` 排序；每筆的 `order` 是拿掉該邊之後的一個拓撲順序。

- 圖本來就沒有環：回傳空的 vector（沒有東西要修）。
- 沒有任何一條邊能單獨解開所有環：也回傳空的 vector。

頂點少、邊很多時，「每條邊都試一次」要 O(E · (V + E))。你要做到 **O(V · (V + E))**。

## 面試追問

1. 為什麼 Stage 3 不必把每條邊都試過？如果要找的是「拿掉兩條邊能變 DAG」的組合，同一個想法還成立嗎？
2. 什麼樣的圖上 BFS 和 DFS 的 `visit_order` 會一模一樣？
3. 把 DFS 改成 iterative 時，為什麼不能在 push 的時候就標記 visited？
