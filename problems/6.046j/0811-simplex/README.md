---
title: Simplex Solver（對偶憑證）
unit: 08 · LP 與 NP-Completeness（PS8）
source: 6.046J PS8 Problem 8-1
pdf: materials/6.046j/psets/ps08/questions.pdf
answer: materials/6.046j/psets/ps08/solutions.pdf
read: materials/6.046j/lectures/lec15.pdf
kind: code
minutes: 180
---

# 0811 · Simplex Solver

> **實務上為什麼會遇到**：整數規劃 solver 的最內圈就是 simplex，大家天天用卻少有人寫過。Simplex 走過的頂點序列、退化時的 cycling、最後吐出的 dual solution，都是除錯 LP 模型時最常要看的東西。

這題把 PS 的手算練習變成一個真正的 solver。所有運算都用 exact rational `Frac`（`_shared/06_fraction.hpp`）。Standard form：`maximize c·x`，受限於 `A x ≤ b`、`x ≥ 0`，`StandardLp{c, A, b}`（`A` 是 m×n）。

一次只看一關。

## Stage 1 — 可行區域的頂點（Part a）

`std::vector<std::pair<Frac,Frac>> feasible_vertices_2d(const StandardLp& lp)`：只考慮 2 個變數的 LP，回傳可行區域所有頂點 `(x, y)`（去重、依字典序升冪）。可行區域為空時回傳空 vector。

提示：候選點來自約束直線、以及 `x = 0`、`y = 0` 兩兩相交。

## Stage 2 — Simplex（Part b、c）

```cpp
enum class LpStatus { Optimal, Infeasible, Unbounded };
struct LpResult {
    LpStatus status = LpStatus::Infeasible;
    Frac value;
    std::vector<Frac> x;      // 長度 n
    std::vector<Frac> dual;   // Stage 4 才用，長度 m
};
LpResult solve_lp(const StandardLp& lp, std::vector<std::vector<Frac>>* trace = nullptr);
```

這關只處理 `b ≥ 0`（原點可行）的情況。用 slack form 反覆 pivot，一律採用 **Bland's rule**（選編號最小的可進基變數；離基選擇同樣 tie-break，選最小 basic 編號）以避免 cycling。

`trace` 非空時，把每次 pivot 後 basic solution 的前 n 個變數值依序寫入（含起點 `0`）。

## Stage 3 — Phase I：初始可行基（Part c 延伸）

`b` 可以有負值。用輔助 LP 找初始可行基，或判定 `Infeasible`。`Unbounded` 時 `solve_lp` 回傳 `status = Unbounded`（`x`/`value` 不必有意義）。

## Stage 4 — 對偶憑證（Part d）

`status == Optimal` 時，`dual` 必須是對偶 LP 的最佳解：`dual ≥ 0`、`Aᵀ·dual ≥ c`、`b·dual == value`。這就是最佳性的 certificate：任何人拿到 `x` 和 `dual`，不必相信你的 solver，只要驗算即可。

## 面試追問

1. Bland's rule 為什麼能避免 cycling？它的代價是什麼？實務 solver 用什麼規則？
2. 退化（degenerate）pivot 的意思是什麼？為什麼目標值不增加時 simplex 不會「卡住」（在 Bland's rule 下）？
3. 為什麼 simplex 最壞情況是指數的（Klee–Minty），實務上卻通常很快？和 interior point 比較呢？
