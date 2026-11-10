---
title: 0-1 Knapsack（滾動陣列與兩種維度的取捨）
unit: 08 · Dynamic Programming II（PS8）
source: 6.006 Recitation 18
pdf: materials/6.006/recitations/rec18.pdf
read: materials/6.006/recitations/rec18.pdf, materials/6.006/lectures/lec18.pdf
kind: code
minutes: 75
---

# 0803 · 0-1 Knapsack

> **實務上為什麼會遇到**：在固定 GPU 時數內挑哪些實驗要跑、在載重內挑哪些 package 要送，或是批次訓練 job 的 admission control，都是 knapsack。容量常常大到填不完一張表，所以知道該用**容量**還是**價值**當 DP 的維度，比記公式重要。

有 `n` 個 item，第 `i` 個的 size 是 `size_i ≥ 1`，value 是 `value_i ≥ 0`，每個最多拿一次，總 size 不超過 `capacity`，要讓總 value 最大。

```cpp
struct Item { std::int64_t size; std::int64_t value; };
```

一次只看一關，做完再往下讀。

## Stage 1 — 最大價值

`std::int64_t knapsack_value(std::span<const Item> items, std::int64_t capacity)`：O(n·capacity) 時間。`capacity ≥ 0`。依價值密度 greedy 是錯的。

## Stage 2 — 挑了哪些

`std::vector<std::size_t> knapsack_items(std::span<const Item> items, std::int64_t capacity)`：回傳達到最大價值的一組 item 索引（遞增），總 size 不可超過 `capacity`。

## Stage 3 — `knapsack_value` 降到 O(capacity) 記憶體

Heap 高水位要在 `capacity` 的線性範圍內。想清楚為什麼內層迴圈的方向會決定 item 能不能被重複使用。

## Stage 4 — 換一個維度

容量可能高達 10¹⁵，但總 value 很小：

`std::int64_t knapsack_by_value(std::span<const Item> items, std::int64_t capacity)`：O(n·V) 時間，`V` 是所有 value 的總和。

以「達到價值 `v` 所需的最小 size」當子問題。

## 面試追問

1. Stage 1 和 Stage 4 都是 pseudopolynomial，差別在哪一個數字要小？怎麼決定用哪個？
2. 把 Stage 3 的內層迴圈方向反過來，算出來的是什麼問題的答案？
3. Knapsack 有 FPTAS。它在時間和精度之間怎麼換？跟 Stage 4 有什麼關係？
