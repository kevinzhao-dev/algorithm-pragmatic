---
title: 精確的期望值（Fraction 與離散分布）
unit: 00 · Warm-up（PS0）
source: 6.006 PS0 Problem 0-2
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 60
---

# 0012 · 精確的期望值

> **實務上為什麼會遇到**：「三副本中至少兩台活著的機率」「重試策略的平均成本」都是離散分布的期望值。用 `double` 累加三十六個 1/36，結果可能不是精確的答案；有些系統（會計、配額計算）一點誤差都不能有。

作業原題要你算擲硬幣、骰子乘積的期望值（數字見 PDF）。這題把「離散分布」做成一個能用的元件：機率用精確的分數，整個計算沒有任何浮點誤差。

## Stage 1 — Fraction

`Fraction` 永遠是最簡分數，分母為正，0 寫成 `0/1`：

- `Fraction()` 為 0；`Fraction(p, q)` 分母為 0 時丟 `std::invalid_argument`。
- `numerator()`、`denominator()`：回傳化簡後的分子與分母。
- `+ - * /`：回傳精確結果；除以零丟 `std::domain_error`。結果放不進 `int64_t` 時丟 `std::overflow_error`。
- `==`、`<=>`：精確比較，分子分母接近 2⁶³ 時交叉相乘不可溢位（用 `__int128`）。

中間值可能超過 64 bit，只要**化簡後的結果**放得進去就要算出來，例如 `(2^62 / 3) · (3 / 2^61) = 2`。

## Stage 2 — 離散分布 Pmf

`Pmf<T>`（probability mass function）：把每個結果對應到機率，`T` 只保證可以比大小。

| 操作 | 行為 |
| --- | --- |
| `Pmf(pairs)` | 從 `(結果, 機率)` 建立；相同結果的機率相加，機率為 0 的結果不算在 support。機率不可為負且總和必須恰為 1，否則丟 `std::invalid_argument` |
| `Pmf::uniform(outcomes)` | 每個位置機率相同（重複的值機率相加） |
| `support()` | 回傳 support 的結果，由小到大 |
| `prob(x)` | `x` 的機率，不在 support 回傳 0 |
| `expected()` | `T` 可轉成 `Fraction` 時的期望值 |
| `expected_of(f)` | `E[f(X)]`，`f` 的回傳值可轉成 `Fraction` |
| `map(f)` | `f(X)` 的分布，回傳 `Pmf<f 的回傳型別>` |

## Stage 3 — 聯合分布與線性

- `independent(a, b)`：回傳 `Pmf<pair<A, B>>`，代表兩個**獨立**隨機變數的聯合分布。
- 測試會用 `expected_of` 驗證 `E[X+Y] = E[X] + E[Y]`（獨立與相依都成立），以及 `E[XY] = E[X]E[Y]` 只在獨立時成立。

## Stage 4 — 實務延伸：quorum 可用性

- `convolve(a, b)`：回傳獨立的 `X + Y` 的分布（`T` 是 `int`）。
- `at_least_k_up(replicas, p_up, k)`：每台機器獨立、活著的機率是 `p_up`，回傳至少 `k` 台活著的精確機率。`k ≤ 0` 回傳 1，`k > replicas` 回傳 0。

## 面試追問

1. `E[X+Y] = E[X]+E[Y]` 不需要獨立，那 `E[XY] = E[X]E[Y]` 呢？請舉一個你自己的反例。
2. 為什麼機率的相乘相加在大型系統裡通常用 `double` 或對數？什麼時候精確分數會爆掉（分子分母的位數怎麼成長）？
3. 真實叢集的機器不是獨立故障（同機架、同一次部署）。相關性故障會讓 `at_least_k_up` 的結果偏樂觀還是悲觀？你會怎麼建模？
