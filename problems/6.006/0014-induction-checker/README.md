---
title: 把歸納法寫成程式（立方和與多項式驗證）
unit: 00 · Warm-up（PS0）
source: 6.006 PS0 Problem 0-4
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 55
---

# 0014 · 把歸納法寫成程式

> **實務上為什麼會遇到**：測試跑過 n = 1…1000 不代表對所有 n 都對。形式驗證、loop invariant、甚至推導一個報表的 closed form，靠的都是同一個結構：base case 加上 inductive step。

作業原題要你用歸納法證明 `1³ + 2³ + … + n³ = (n(n+1)/2)²`（PDF 第 1 頁）。這題把這個結論用起來，再把「歸納法」本身變成可以執行的檢查。**手寫證明的部分放在面試追問。**

## Stage 1 — 用 closed form

- `checked_sum_of_cubes(n)`：回傳 `1³ + … + n³`，放不進 `uint64_t` 時回傳 `std::nullopt`，`n` 可能非常大。
- `sum_of_cubes_mod(n, m)`：回傳 `(1³ + … + n³) mod m`。`n` 可到 4 × 10¹⁸，`m ≥ 1`，每次呼叫必須是 **O(1)**。

注意 `n(n+1)/2` 要先除以 2 再取模，也要小心溢位。

## Stage 2 — 整數係數多項式

`Poly`，`coefficients()[i]` 是 `xⁱ` 的係數；領導係數不為 0（零多項式就是空的 `coefficients()`，`degree()` 為 −1）：

| 操作 | 行為 |
| --- | --- |
| `Poly(coeffs)` | 建構並去除尾端的 0 |
| `p(x)` | 求值，用 Horner 法則 |
| `+`、`-`、`*`、`==` | 多項式的加、減、乘、比較 |
| `shifted()` | 回傳多項式 `q(x) = p(x + 1)` |

所有係數與求值結果都保證放得進 `int64_t`。

## Stage 3 — 歸納法檢查器

`check_sum_formula(term, numerator, denominator)` 檢查這個斷言：

> 對所有整數 `n ≥ 1`：`term(1) + term(2) + … + term(n) == numerator(n) / denominator`

回傳 `InductionReport`：

- `base_case`：`n = 1` 時斷言是否成立。
- `inductive_step`：對**所有** `k`，由 `n = k` 成立能推出 `n = k + 1` 成立。
- `proved()`：兩者都成立。

**不可以只拿幾個 n 去試。** Inductive step 必須是多項式恆等式的檢查（`numerator(x+1) − numerator(x)` 與 `denominator · term(x+1)` 是否為同一個多項式），這樣結論才對所有 n 都成立。`denominator == 0` 時丟 `std::invalid_argument`。

## 面試追問

1. 口頭走一遍 `Σ i³ = (n(n+1)/2)²` 的完整歸納證明：base case 是什麼？inductive step 怎麼把 `k+1` 的情況拆回 `k`？
2. `inductive_step` 是 true 但 `base_case` 是 false，這個式子代表什麼？為什麼「只要 inductive step 就夠了」是錯的？
3. 兩個次數 ≤ d 的多項式，在幾個點相同就保證完全相同？這跟隨機測試（polynomial identity testing）有什麼關係？
