---
title: Rod Cutting、Unbounded Knapsack 與 Coin Change
unit: 08 · Dynamic Programming II（PS8）
source: 6.006 Lecture 18, Recitation 18
pdf: materials/6.006/lectures/lec18.pdf
read: materials/6.006/lectures/lec18.pdf, materials/6.006/recitations/rec18.pdf
kind: code
minutes: 80
---

# 0801 · Rod Cutting、Unbounded Knapsack 與 Coin Change

> **實務上為什麼會遇到**：買 reserved capacity、拼湊符合需求的規格、換零錢，都是「同一種東西可以重複用」的整數分解。子問題就是一個比目前小的整數，所以時間是 pseudopolynomial：資料多大不是看有幾個數字，而是看**數字本身多大**。

一次只看一關，做完再往下讀。

## Stage 1 — Rod Cutting 的最大價值

`std::int64_t rod_cut_value(std::span<const std::int64_t> prices)`：`prices[l]` 是長度 `l` 的一段能賣的價錢（`prices[l] ≥ 0`，`prices[0]` 不使用），整根長度 `L = prices.size() − 1`，切成整數長度的段，回傳最大總價。空輸入或 `L == 0` 回傳 0。O(L²) 時間。

單位長度最貴的切法是**錯的**。

## Stage 2 — 切法

`std::vector<std::size_t> rod_cut_pieces(std::span<const std::int64_t> prices)`：回傳每一段的長度（順序不拘），總和要等於 `L`，總價要是最大值。

## Stage 3 — 只有某些長度能賣（Unbounded Knapsack）

```cpp
struct Item { std::size_t size; std::int64_t value; };   // size >= 1, value >= 0
std::int64_t unbounded_knapsack(std::span<const Item> items, std::size_t capacity);
```

每種 item 可以用任意多次，總 size 不超過 `capacity`，回傳最大總 value。O(n·capacity) 時間，O(capacity) 記憶體。

## Stage 4 — Coin Change 兩種計數

```cpp
std::optional<std::int64_t> min_coins(std::span<const int> coins, int amount);  // 湊不出來時 nullopt
std::uint64_t count_change_ways(std::span<const int> coins, int amount);        // 不計順序
std::uint64_t count_ordered_ways(std::span<const int> coins, int amount);       // 計順序
```

面額皆為正，同一種面額可重複使用，`amount ≥ 0`（`amount == 0` 時最少 0 枚，兩種方法數都是 1）。兩個計數函式的差別只在迴圈順序，想清楚為什麼。

## Stage 5 — 實務延伸：買足夠的產能

```cpp
struct Offer { std::int64_t units; std::int64_t price; };   // units >= 1, price >= 0
struct Purchase { std::int64_t cost; std::vector<std::int64_t> counts; };

std::optional<Purchase> cheapest_capacity(std::span<const Offer> offers, std::int64_t demand);
```

每種方案可以買任意多份，總 `units` **至少**要達到 `demand`。回傳最低成本和每種方案買幾份（`counts[i]` 對應 `offers[i]`，買超過也可以）。`demand` 為 0 時成本 0、全部買 0 份；沒有 offer 又需要買時回傳 `std::nullopt`。

## 面試追問

1. Rod Cutting 的 O(L²) 算 polynomial 嗎？Stage 3 的 O(n·capacity) 呢？差別在哪？
2. 什麼面額系統（例如現行硬幣）下 greedy 剛好最佳？怎麼判斷？
3. `count_change_ways` 和 `count_ordered_ways` 把內外迴圈對調，對應的子問題定義分別是什麼？
