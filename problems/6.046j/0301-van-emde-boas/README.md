---
title: van Emde Boas Tree
unit: 03 · van Emde Boas（PS3）
source: 6.046J Lecture 4
pdf: materials/6.046j/lectures/lec04.pdf
read: materials/6.046j/lectures/lec04.pdf
kind: code
minutes: 240
---

# 0301 · van Emde Boas Tree

> **實務上為什麼會遇到**：路由表要在固定大小的位址空間裡找「不大於這個 IP 的最大起點」。平衡樹要 O(log n)，vEB 把時間壓到 O(log log u)，跟集合大小無關。這是少數能打破比較模型下界的資料結構之一。

維護 `0..u-1` 中的整數集合，`u = 2^bits`，`1 ≤ bits ≤ 64`（否則丟 `std::invalid_argument`）。key 超出範圍時丟 `std::out_of_range`。

把 key 切成兩段：高 `bits - bits/2` 位是 cluster 編號，低 `bits/2` 位是 cluster 內的位置。結構如 Lecture 4：每個節點有 `min`、`max`、`summary` 和各個 `cluster`，而且 **`min` 不存進 cluster**。把 `bits == 1` 當 base case。

## 成本模型

`visits()` 回傳累積進入過多少個 vEB 節點，也就是每一次遞迴呼叫（包含對根的呼叫）都要 `+1`，只讀 `min`、`max` 不算。測試用它驗證 T(u) = T(√u) + O(1)。每個操作最多可以進入 `2·L + 2` 個節點，`L` 是遞迴層數 `⌈log₂ bits⌉ + 1`。

## Stage 1 — 新增與後繼

`insert`（新增回傳 `true`，已存在回傳 `false`）、`contains`、`min`、`max`、`size`、`empty`，以及 `successor(x)`：回傳**嚴格大於** `x` 的最小元素，沒有時回傳 `std::nullopt`。

這一關的 cluster 可以用陣列，但不要在這裡寫死 16 位元，Stage 3 會換成 64。注意 `successor` 只有一條路：要嘛遞迴進 cluster，要嘛遞迴進 summary，不能兩邊都做。

## Stage 2 — 刪除與前驅

- `erase(x)`：存在時刪除並回傳 `true`。`min` 被刪時，要從第一個非空 cluster 拉新的 `min` 上來；`max` 被刪時要從 summary 重建。
- `predecessor(x)`：回傳嚴格小於 `x` 的最大元素。別忘了 `min` 不在 cluster 裡。

## Stage 3 — 稀疏宇集

把 cluster 改成只保存非空的（`std::unordered_map`），讓 `bits = 64` 也能跑：

- `node_count()`：目前存活的 vEB 節點數（包含根），必須是 `O(n log log u)`。
- 所有元素刪光後，節點要全部釋放，`node_count()` 回到 `1`。

注意 `1ULL << 64` 是 undefined behavior，UBSan 會抓。

## Stage 4 — 實務延伸：路由表

`RouteTable`：路由是 `[first, last]` 這段 IPv4 位址對應到 `port`。

| 操作 | 行為 |
| --- | --- |
| `add(first, last, port)` | 和現有路由重疊時回傳 `false` 且不變動；`first > last` 丟 `std::invalid_argument` |
| `remove(first)` | 刪除起點剛好是 `first` 的路由，回傳是否有這條路由 |
| `lookup(ip)` | 回傳包含 `ip` 的路由的 `port` |
| `size()` | 路由數量 |

用 `VanEmdeBoas(32)` 存起點，每個操作的時間跟路由數無關。

## 面試追問

1. 不把 `min` 存進 cluster 這件事，為什麼讓 `insert` 的遞迴只需要走一邊？
2. 用 `unordered_map` 之後，T(u) = T(√u) + O(1) 還成立嗎？最壞情況呢？
3. 實務上 vEB 比 `std::set` 或 B-tree 慢的原因是什麼？什麼情況下才真的划算？
