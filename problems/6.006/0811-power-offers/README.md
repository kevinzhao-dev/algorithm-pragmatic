---
title: 賣出定量的電（0-1 Knapsack 加懲罰）
unit: 08 · Dynamic Programming II（PS8）
source: 6.006 PS8 Problem 8-1
pdf: materials/6.006/psets/ps8/questions.pdf
answer: materials/6.006/psets/ps8/solutions.pdf
read: materials/6.006/recitations/rec18.pdf, materials/6.006/lectures/lec18.pdf
kind: code
minutes: 65
---

# 0811 · 賣出定量的電

> **實務上為什麼會遇到**：再生能源電廠的發電量是固定的，賣不掉要付 curtailment 或儲能成本，負電價也是真實發生的現象；雲端的 spot capacity 也有一樣的結構。把買方報價、固定產能和閒置成本放在一起，就是一個帶懲罰的 0-1 knapsack。

本週可售出 `capacity` 單位（`capacity ≥ 0`）。有 `n` 張訂單，第 `i` 張要買 `amount_i` 單位，**總價**是 `price_i`（注意是總價，不是單價，而且可能是負的：你付錢請人收走）。

```cpp
struct Order { std::int64_t amount; std::int64_t price; };   // amount >= 1
```

每張訂單要嘛完整履行、要嘛完全不做，不可超賣，每張最多做一次。沒賣出的每單位要付 `storage_cost` 元（`storage_cost ≥ 0`）。

利潤 = 履行訂單的總價 − `storage_cost` ×（沒賣出的單位數）。利潤可能是負的。

一次只看一關，做完再往下讀。

## Stage 1 — 最大利潤

`std::int64_t max_profit(std::span<const Order> orders, std::int64_t capacity, std::int64_t storage_cost)`：O(n·capacity) 時間。

Base case 不再是 0，想清楚沒有訂單時的值是什麼。

## Stage 2 — 要接哪些訂單

`std::vector<std::size_t> chosen_orders(orders, capacity, storage_cost)`：回傳達到最大利潤的一組訂單索引（遞增），總量不可超過 `capacity`。

## Stage 3 — 實務延伸：整條容量曲線

Solver 的 DP 表其實已經回答了「如果我只有 `j` 單位，能賺多少」。實作

`std::vector<std::int64_t> profit_curve(orders, capacity, storage_cost)`：回傳 `capacity + 1` 個值，`result[j]` 是產能恰好為 `j` 單位時的最大利潤。O(n·capacity) 時間，O(capacity) 記憶體。並且把 `max_profit` 降到 O(capacity) 記憶體（Heap 高水位要在 `capacity` 的線性範圍內）。

## 面試追問

1. 把這題跟一般的 0-1 knapsack 比較：哪些地方一樣，哪個 base case 不一樣？負價的訂單為什麼有可能值得接？
2. 這個演算法對輸入大小是 polynomial、pseudopolynomial 還是 exponential？為什麼？
3. `profit_curve` 的相鄰差值代表什麼？業務上可以怎麼用（例如決定要不要多買一單位產能）？
