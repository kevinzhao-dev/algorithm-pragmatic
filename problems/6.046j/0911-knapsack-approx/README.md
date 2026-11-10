---
title: Knapsack：Greedy、2-Approx、DP 與 FPTAS
unit: 09 · Approximation 與 FPT（PS9）
source: 6.046J PS9 Problem 9-1
pdf: materials/6.046j/psets/ps09/questions.pdf
answer: materials/6.046j/psets/ps09/solutions.pdf
read: materials/6.046j/lectures/lec17.pdf
kind: code
minutes: 140
---

# 0911 · Knapsack：從 Greedy 到 FPTAS

> **實務上為什麼會遇到**：廣告版位的 budgeted selection、選哪些 model checkpoint 上線、容量受限的快取置換，全都是 knapsack。工程上常見的路線正是這四步：先上 greedy，發現翻車，換 2-approx，再用 FPTAS 換精度。

`Item{size, value}`，都是正整數。`capacity` 是背包容量 `B`（正整數）；除非特別說明，保證 `size ≤ B`。`Selection` 回傳選中的索引（`items`，升冪）、總 `value`、總 `size`。一次只看一關。

## Stage 1 — Density Greedy 為什麼不夠（Part a）

`Selection greedy_by_density(std::span<const Item> items, long long capacity)`：依 `value/size` 由大到小（相同時 index 小者優先），單趟掃描，放得下就放。比較 density 時不要用浮點數（交叉相乘）。

`DensityTrap density_trap(long long k)`，`DensityTrap{items, capacity}`：給定 `k ≥ 1`，構造一個輸入，使 greedy 的總價值 × k ≤ OPT（兩個物品就夠）。

## Stage 2 — 2-Approximation（Part b）

`Selection greedy_two_approx(std::span<const Item> items, long long capacity)`：題目 `Alg2`：若全部放得下則全放；否則依 density 排序（同 Stage 1 的順序），找到第一個使前綴總 size 超過 `B` 的項目 `a_i`，比較 `v_i` 與前綴價值：`v_i > Σ_{j<i} v_j` 時回傳 `{a_i}`，否則回傳 `{a_1, …, a_{i-1}}`。保證 `value ≥ OPT/2`。

## Stage 3 — Exact DP by Value（Part c）

`Selection exact_by_value(std::span<const Item> items, long long capacity)`：題目 `Alg3`：`S[i][v]` 是前 `i` 個物品恰好湊出價值 `v` 的最小總 size。時間 O(n²V)，其中 `V` 是最大的單項 value。`size > capacity` 的項目可直接忽略。

## Stage 4 — FPTAS（Part d）

`Selection knapsack_fptas(std::span<const Item> items, long long capacity, double eps)`：題目 `Alg4`。縮放 `v'_i = ⌊v_i · n / (V · ε)⌋` 後對 `v'` 跑 Stage 3，回傳的集合要符合**原始的** size 限制，且 `value ≥ (1-ε)·OPT`。`0 < ε < 1`。時間 O(n³/ε)。

## 面試追問

1. Greedy 排序加 Part b 的「比較最後一個放不下的物品」，為什麼變成常數近似？這跟 fractional knapsack 的 upper bound 有什麼關係？
2. Alg3 為什麼是 pseudo-polynomial？它和 O(nB) 的 DP 各在什麼情況下比較好？
3. FPTAS 的誤差來自哪裡？如果把 `ε` 設成 `1/n`，時間變成什麼樣？這跟 P vs NP 有什麼關係（為什麼這不是多項式解）？
