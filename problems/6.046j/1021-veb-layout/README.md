---
title: van Emde Boas Layout（Cache-Oblivious Search）
unit: 10 · Distributed Algorithms（PS10）
source: 6.046J Lecture 24
pdf: materials/6.046j/lectures/lec24.pdf
read: materials/6.046j/lectures/lec23.pdf, materials/6.046j/lectures/lec24.pdf
kind: perf
minutes: 110
---

# 1021 · van Emde Boas Layout

> **實務上為什麼會遇到**：靜態索引（log-structured merge 的 SSTable、read-only 字典、routing table）每次查詢的成本不是比較次數，而是 cache miss。B-tree 要知道 block 大小 B；vEB layout 不知道 B 也能達到 O(log_B n)，這就是 cache-oblivious 的魔法（和 6.172 同一個主題）。

一次只看一關。Key 為 `long long`，輸入 `sorted_keys` 嚴格遞增。

## Stage 1 — 建出 vEB Layout

`std::vector<VebNode> build_veb_layout(std::span<const long long> sorted_keys)`，`VebNode{key, left, right}`（`left`、`right` 是子節點在回傳 vector 中的 index，無子節點為 `-1`）。

1. 樹的形狀：以 median 分割遞迴建成的平衡 BST（高度 `h = ⌈lg(n+1)⌉`）。
2. 記憶體順序：**van Emde Boas layout**：把樹從高度中間切開，上半部 `⌊h/2⌋` 層遞迴排在前面，下面各個子樹再依左到右遞迴排在後面。root 必須在 index 0。

`n = 0` 回傳空 vector。

## Stage 2 — 用 block 計算查詢成本

沒有 cachegrind，所以我們用確定性的 block 模型量成本：

`int search_blocks(std::span<const VebNode> nodes, long long key, std::size_t block_bytes)`：從 root（index 0）開始做 BST 搜尋（命中或走到 `-1` 為止），回傳經過的節點橫跨了多少個**相異的 block**。節點 `i` 占 bytes `[i·sizeof(VebNode), (i+1)·sizeof(VebNode))`，block `b` 是 bytes `[b·block_bytes, (b+1)·block_bytes)`（`block_bytes` 是 `sizeof(VebNode)` 的倍數）。

`std::vector<VebNode> build_bfs_layout(std::span<const long long> sorted_keys)`：同樣形狀的 BST，改成一般的 BFS（level order）layout，當作 baseline。

測試會在 `n = 2²⁰` 上比較兩種 layout 的平均 block 數。

## Stage 3 — 實務延伸：唯讀索引

`VebIndex(std::vector<long long> sorted_keys)`：

- `bool contains(long long key) const`
- `std::optional<long long> predecessor(long long key) const`：不大於 `key` 的最大鍵，不存在時 `nullopt`。

O(log n) 比較。

## 面試追問

1. 為什麼 vEB layout 每條 root-to-leaf 路徑只碰到 O(log_B n) 個 block？切成 `√n` 大小的子樹的直覺是什麼？
2. Binary search 的 O(log(n/B)) 為什麼比 O(log_B n) 差？B-tree 為什麼需要知道 B？
3. 把這個概念用到 dynamic 情況（cache-oblivious B-tree，packed memory array）時，主要難在哪？
