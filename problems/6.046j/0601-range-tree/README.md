---
title: 2D Range Tree（矩形範圍查詢）
unit: 06 · Augmentation、APSP 與 MST（PS6）
source: 6.046J Lecture 9
pdf: materials/6.046j/lectures/lec09.pdf
read: materials/6.046j/lectures/lec09.pdf
kind: code
minutes: 120
---

# 0601 · 2D Range Tree

> **實務上為什麼會遇到**：監控系統問「過去一小時、latency 落在 200–500 ms 的請求有幾筆、傳了多少 bytes」，遊戲伺服器問「這個矩形裡有哪些單位」。這些都是點集合上的 axis-aligned 範圍查詢；線性掃描在十萬點、十萬查詢下就撐不住。

點集合建好後不再變動（static），所以可以花 O(n log n) 空間換查詢速度。所有座標型別 `T` 只保證可複製與 `<`（其他比較運算子也有）；測試會用 `practice::Counted<int>` 數比較次數，因此複雜度是以「比較次數」驗證的。

一次只看一關，做完再往下讀。

## Stage 1 — 1D range tree 與 canonical 分解

`RangeTree1D<T>(std::vector<T> keys)`：鍵值可重複，索引指向建構時的輸入位置。

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `count(a, b)` | 有多少鍵滿足 `a ≤ key ≤ b` | O(log n) |
| `report(a, b)` | 回傳這些鍵的原始索引，順序不限 | O(log n + k) |
| `canonical(a, b)` | 回傳 O(log n) 段排序位置的 half-open 區間 `[l, r)`，互不相交、聯集恰好是答案 | O(log n) |

`a > b` 時範圍是空的。`canonical` 就是講義 "nodes and subtrees in between" 的陣列版本：段數不可超過 `2⌈lg n⌉ + 2`。

邊界：空集合、全部鍵相同、查詢完全沒涵蓋任何鍵。

## Stage 2 — 2D range tree

`RangeTree2D<T>(std::vector<Point<T>> pts)`，其中 `Point<T>{x, y}`。

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `count(x1, x2, y1, y2)` | 閉矩形內的點數，`x1 > x2` 或 `y1 > y2` 時為 0 | O(log² n) |
| `report(x1, x2, y1, y2)` | 回傳這些點的原始索引，順序不限 | O(log² n + k) |

建構 O(n log n)，空間 O(n log n)。

## Stage 3 — 實務延伸：帶權範圍聚合

`WeightedRangeTree<T, V>(std::vector<WeightedPoint<T, V>> pts)`，其中 `WeightedPoint<T, V>{x, y, weight}`，`V` 是可做加減的數值型別。

- `V sum(x1, x2, y1, y2) const`：閉矩形內所有點的 weight 總和，空矩形回傳 `V{}`，O(log² n)。
- `std::size_t count(x1, x2, y1, y2) const`：同上回傳點數。

這正是「指定 timestamp × latency 兩個維度，查詢總流量」的 ad hoc 查詢元件。

## 面試追問

1. 為什麼不能直接把 rank 存在節點裡？動態 insert 會發生什麼事？
2. Fractional cascading 怎麼把 2D 查詢從 O(log² n) 降到 O(log n)？要多少額外空間？
3. 需要支援 insert 時，你會選 logarithmic method（Bentley–Saxe）還是平衡樹套平衡樹？各自的最壞情況成本？
