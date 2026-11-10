---
title: 同餘運算（floor mod、模冪、週期撞期）
unit: 00 · Warm-up（PS0）
source: 6.006 PS0 Problem 0-3
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 50
---

# 0013 · 同餘運算

> **實務上為什麼會遇到**：環狀緩衝區的 index、hash 分片、時間窗口切桶都在做 mod，而 C++ 的 `%` 遇到負數會給你負的餘數。雜湊（如 Rabin-Karp）要算模冪，兩個週期性 job 何時撞期則是同餘方程。

作業原題要你判斷幾組同餘式的真假（數字見 PDF 第 1 頁）。這題把 mod 寫成可以信賴的元件。

## Stage 1 — 不被負數騙

模數 `m` 必須 > 0，否則丟 `std::invalid_argument`：

- `floor_mod(a, m)`：回傳 `[0, m)` 中與 `a` 同餘的值。
- `floor_div(a, m)`：回傳 `⌊a / m⌋`（往負無限大取整）。必須滿足 `a == m * floor_div(a, m) + floor_mod(a, m)`。
- `congruent(a, b, m)`：`a ≡ b (mod m)` 是否成立。要小心：直接算 `a - b` 可能溢位。

## Stage 2 — 模冪

- `mul_mod(a, b, m)`：`(a · b) mod m`，`a, b, m` 都可以接近 2⁶⁴（用 128-bit 中間值），`m ≥ 1`。
- `pow_mod(base, exponent, m)`：`base^exponent mod m`，`exponent` 可到 2⁶³ 以上，要 **O(log exponent)** 次乘法。`m ≥ 1`，`0^0` 當作 1。

## Stage 3 — 實務延伸：兩個週期何時撞期

資料中心裡有兩個週期性 batch job，常要避開彼此撞期。

`first_coincidence(a, b)`：`a`、`b` 是 `PeriodicJob{phase, period}`，表示 job 在時間 `phase + k·period`（`k ≥ 0`）執行，`phase ≥ 0`、`period ≥ 1`。回傳兩者第一次同時執行的時刻；永遠不會撞期時回傳 `std::nullopt`。

- 週期不一定互質。
- 週期可到 10⁹、phase 可到 10¹²：逐一枚舉執行時間會太慢（有 `PERF` 檢查）。這一關用 extended Euclid（貝祖係數）。

## 面試追問

1. 為什麼環狀緩衝區常要求容量是 2 的冪次？`%` 和 `&` 的差別，對負數又是怎麼表現？
2. 什麼時候 `first_coincidence` 無解？用 gcd 說明原因。
3. Fermat 小定理可以拿來做什麼（模反元素、Miller-Rabin）？模數不是質數時要怎麼辦？
