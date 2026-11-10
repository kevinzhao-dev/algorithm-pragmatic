---
title: Burst 排程與冷卻（狀態式線性 DP）
unit: 07 · Dynamic Programming I（PS7）
source: 6.006 PS7 Problem 7-1
pdf: materials/6.006/psets/ps7/questions.pdf
answer: materials/6.006/psets/ps7/solutions.pdf
read: materials/6.006/lectures/lec15.pdf
kind: code
minutes: 70
---

# 0711 · Burst 排程與冷卻

> **實務上為什麼會遇到**：GPU 和 CPU 的 boost 時脈、雲端的 burst credit、API 的 cooldown，都是「現在多做一點，接下來幾個時間片只能維持基本水準」。排程器得在事前決定什麼時候 burst 才划算，而不是能 burst 就 burst。

有 `n` 個連續的時間片，第 `i` 片平常可以處理 `base` 單位，burst 時可以處理 `burst` 單位（`0 ≤ base < burst`）。如果在第 `i` 片 burst，**接下來的 `cooldown` 片都不能 burst**，只能處理各自的 `base`。

```cpp
struct Slot { std::int64_t base; std::int64_t burst; };
```

一次只看一關，做完再往下讀。

## Stage 1 — 最大總處理量（`cooldown = 2`）

```cpp
std::int64_t max_throughput(std::span<const Slot> slots, std::size_t cooldown = 2);
bool can_meet_quota(std::span<const Slot> slots, std::int64_t quota, std::size_t cooldown = 2);
```

`can_meet_quota`：能不能讓總處理量達到 `quota`。O(n) 時間。這一關只需要處理預設的 `cooldown`。

## Stage 2 — 排程表

`std::vector<std::size_t> burst_plan(std::span<const Slot> slots, std::size_t cooldown = 2)`：回傳要 burst 的時間片（遞增），總處理量要等於最佳值。同分時任一種都可以。

## Stage 3 — 任意 cooldown，O(n) 時間、O(min(n, cooldown)) 記憶體

現在 `cooldown` 可以是任意值（`0` 表示沒有限制，也可能比 `n` 大）。`max_throughput` 的時間要維持 O(n)，不可以隨 `cooldown` 變慢；額外記憶體要在 O(min(n, cooldown)) 以內。`burst_plan` 也要支援任意 `cooldown`，時間 O(n)、記憶體不受此限。

## Stage 4 — 實務延伸：burst 額度

`std::int64_t max_throughput_limited(std::span<const Slot> slots, std::size_t cooldown, std::size_t max_bursts)`：同時限制總共最多 burst `max_bursts` 次。O(n · max_bursts) 時間，記憶體 O(n)。

## 面試追問

1. 這題要從 `quota` 的角度判定可行性，會比先求最大值再比較更快嗎？為什麼「能不能達標」和「最大能多少」在這裡是同一件事？
2. Stage 3 為什麼可以只留最近 `cooldown + 1` 個子問題？如果 cooldown 依時間片而異（每片不同），這個省記憶體的手法還成立嗎？
3. 這是 polynomial 還是 pseudopolynomial？Stage 4 呢？（`max_bursts` 不必大於 `n`。）
