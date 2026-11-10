---
title: 依優先序把工作放到機器上（序列對齊 DP）
unit: 07 · Dynamic Programming I（PS7）
source: 6.006 PS7 Problem 7-2
pdf: materials/6.006/psets/ps7/questions.pdf
answer: materials/6.006/psets/ps7/solutions.pdf
read: materials/6.006/lectures/lec16.pdf
kind: code
minutes: 70
---

# 0712 · 依優先序把工作放到機器上

> **實務上為什麼會遇到**：叢集排程常要求「比較重要的工作一定要放到比較近、比較快的機器」，同時又要盡量少超賣資源。高優先序配低延遲這條規則把問題壓成一維順序，於是不用解通用的 assignment problem，一張 LCS 形狀的表就夠了。

有 `n` 個工作和 `m` 台機器（`m ≥ n`，可能遠大於 `n`）。每個工作恰好放一台機器，每台機器最多放一個工作。

```cpp
struct Task { int priority; int demand; };      // all priorities distinct; larger = more important
struct Server { int latency; int capacity; };   // all latencies distinct
```

- **順序規則**：若工作 `a` 的 `priority` 大於工作 `b`，那 `a` 所在機器的 `latency` 必須**嚴格小於** `b` 的。
- **超賣成本**：工作放到容量 `c` 的機器上，成本是 `max(0, demand − c)`。

目標是在遵守順序規則下，總成本最小。一次只看一關，做完再往下讀。

## Stage 1 — 最小成本

`std::optional<std::int64_t> min_overload(std::span<const Task> tasks, std::span<const Server> servers)`：O(n·m) 時間（排序除外）。`n > m` 時回傳 `std::nullopt`；沒有工作時回傳 0。

先想清楚：排好序之後，答案長得像「在一個序列裡挑出子序列去對齊另一個序列」。

## Stage 2 — 實際配置

`std::optional<std::vector<std::size_t>> assign_servers(tasks, servers)`：回傳的 vector 中第 `i` 個值是 `tasks[i]` 配到的機器索引（依輸入順序，不是排序後的順序）。總成本要是最小值，同分時任一種都可以。

## Stage 3 — `min_overload` 降到 O(m) 記憶體

Relation 只看下一列，所以記憶體不該隨 `n` 成長。Heap 高水位要在 O(m) 以內。

## Stage 4 — 實務延伸：允許丟棄工作

機器不夠或成本太高時，排程器會直接拒絕工作：

```cpp
struct DropPlan { std::int64_t cost; std::vector<std::optional<std::size_t>> assignment; };
DropPlan plan_with_drops(std::span<const Task> tasks, std::span<const Server> servers,
                         std::span<const std::int64_t> drop_penalty);
```

`drop_penalty[i] ≥ 0` 是丟棄 `tasks[i]` 的代價，`assignment[i]` 是配到的機器或 `std::nullopt`。**已配置的工作之間**仍要遵守順序規則。回傳總代價最小的方案，`n > m` 也要能處理。O(n·m) 時間。

## 面試追問

1. 為什麼順序規則能讓問題從通用的 assignment（Hungarian，O(n³) 以上）簡化成 O(n·m)？去掉規則後這個 DP 還成立嗎？
2. Stage 2 要還原解，但表太大時，你會怎麼縮小 parent 的儲存（一個 cell 需要幾個 bit）？
3. `m` 是輸入大小的一部分，這個演算法是 polynomial 還是 pseudopolynomial？
