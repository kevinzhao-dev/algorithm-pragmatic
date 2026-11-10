---
title: Release Planning（四種帶限制的挑選 DP）
unit: 05 · Skip Lists、Hashing 與 DP（PS5）
source: 6.046J PS5 Problem 5-2
pdf: materials/6.046j/psets/ps05/questions.pdf
answer: materials/6.046j/psets/ps05/solutions.pdf
read: materials/6.046j/lectures/lec01.pdf, materials/6.046j/lectures/lec02.pdf
kind: code
minutes: 180
---

# 0512 · Release Planning

> **實務上為什麼會遇到**：一次發版最多能上 `m` 項變更，每項有不同收益，但上線順序和相依關係會限制哪些組合可行。同樣的預算問題換幾個限制，適用的演算法就完全不同。

每項變更的收益是非負整數，有負值時丟 `std::invalid_argument`。「最多選 `m` 項」，`m` 可以是 0，也可以大於項目數。每個函式都回傳**遞增排序**的索引，最佳解不唯一時回傳任一組，總收益要最大。

## Stage 1 — 沒有限制

`pick_top(values, m)`：挑收益最高的最多 `m` 項。O(n) 或 O(n log m)，不要把整個陣列排序後再排回原順序。

## Stage 2 — 先遷移，後上線

`pick_a_then_b(prizes, m)`：`type` 只會是 `'A'`（schema 遷移）或 `'B'`（服務上線），其他值丟 `std::invalid_argument`。在選出的項目中，所有 `A` 都必須排在所有 `B` 之前（依原本的索引順序）。O(nm) 時間。

子問題要分兩種狀態：還只在選 `A`，或已經可以選 `B`。回傳實際的選擇，要記錄足夠的資訊才能回溯。

## Stage 3 — 影響範圍只能變大

`pick_non_decreasing(values, m)`：選出的項目，收益依索引順序必須**非遞減**。這一關先求對，任何多項式時間都接受，Stage 5 才要求效能。

## Stage 4 — 功能相依

`pick_rooted_subtree(tree, root, m)`：`tree` 是二元樹，`tree[u].left`、`right` 是子節點索引，沒有時是 `-1`。除了 `root`，選了某個節點就必須也選它的父節點，也就是選出的節點要是包含 `root` 的連通子樹（或者完全不選）。

空樹回傳空，此時 `root` 不看。樹非空且 `root` 超出範圍時丟 `std::invalid_argument`。可能是五十萬個節點的一條長鏈，不可以遞迴。

## Stage 5 — 壓低複雜度

沒有新的介面，只有 `PERF` 測試：

- `pick_non_decreasing` 要 **O(nm log n)**。對固定長度，「結尾值不超過 `x` 的最佳總和」是個前綴最大值查詢。
- `pick_rooted_subtree` 要 **O(nm)**。每個節點的表只需要 `min(子樹大小, m) + 1` 項，合併兩個子節點的成本是兩張表長度的乘積。

## 面試追問

1. Stage 2 的兩個狀態為什麼夠用？如果類型順序變成 `A < B < C`，或者要交替出現，狀態要怎麼擴充？
2. 為什麼 Stage 5 的樹上合併加總起來只有 O(nm)，而不是 O(nm²)？（把每次乘積想成在數一對節點。）
3. Stage 3 的 DP 要保留回溯資訊時，空間成本是多少？只要總和不要選擇時可以省到多少？
