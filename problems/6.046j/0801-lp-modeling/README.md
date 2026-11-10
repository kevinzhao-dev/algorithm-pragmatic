---
title: LP 建模（Standard Form、Slack Form、對偶）
unit: 08 · LP 與 NP-Completeness（PS8）
source: 6.046J Lecture 15
pdf: materials/6.046j/lectures/lec15.pdf
read: materials/6.046j/lectures/lec15.pdf
kind: code
minutes: 110
---

# 0801 · LP 建模

> **實務上為什麼會遇到**：廣告預算配置、生產排程、網路流量工程，第一步都是把商業問題寫成 LP。寫錯 standard form 的方向、漏掉一個 slack，solver 就會給你一個數學上正確、但完全不是你要的答案。

所有數值都是 exact rational，`Frac`（`_shared/06_fraction.hpp`，已提供）。LP 用的 standard form 是：`maximize c·x`，受限於 `A x ≤ b`、`x ≥ 0`。

一次只看一關。

## Stage 1 — 轉成 Standard Form

```cpp
struct GeneralLp {
    bool maximize = true;
    std::vector<Frac> objective;
    std::vector<GeneralConstraint> constraints;   // GeneralConstraint{coeffs, relation, rhs}
    std::vector<bool> nonnegative;                // nonnegative[j] == false 表示 x_j 無正負限制
};
```

- `StandardForm to_standard(const GeneralLp& lp)`：回傳 `StandardForm`，其中 `.lp` 是 `StandardLp{c, A, b}`（標準形式的 LP）。依講義的四條規則轉換（minimize、free variable、equality、≥）。需要的話可以在 `StandardForm` 裡加欄位。
- `std::vector<Frac> recover_solution(const StandardForm& form, std::span<const Frac> x)`：把 standard LP 的解 `x` 還原成原問題 `n` 個變數的解。
- `Frac recover_value(const StandardForm& form, const Frac& standard_value)`：把 standard LP 的目標值還原成原問題的目標值。

## Stage 2 — Slack Form 與 Pivot

```cpp
struct SlackForm {
    std::vector<int> nonbasic, basic;      // 變數編號
    std::vector<std::vector<Frac>> a;
    std::vector<Frac> b;
    std::vector<Frac> c;                   // 目標 z = v + Σ_j c[j]·x_nonbasic[j]
    Frac v;
};
```

- 方程式：`x_basic[i] = b[i] - Σ_j a[i][j]·x_nonbasic[j]`。
- `SlackForm to_slack(const StandardLp& lp)`：變數 `0..n-1` 為 nonbasic，slack `n..n+m-1` 為 basic（`basic[i] = n + i`）。
- `SlackForm pivot(const SlackForm& form, int e, int l)`：`e` 是 `nonbasic` 裡的位置，`l` 是 `basic` 裡的位置，`a[l][e] != 0`。回傳交換 `nonbasic[e]` 與 `basic[l]` 後的等價 slack form（其餘順序不變，新 basic 留在 `basic[l]`，新 nonbasic 留在 `nonbasic[e]`）。

## Stage 3 — 建模：Max Flow 與 Shortest Paths

- `GeneralLp max_flow_lp(int n, std::span<const FlowEdge> edges, int s, int t)`：變數是每條邊的流量（`FlowEdge{u, v, capacity}`，建構順序即變數順序），最大化流量值。
- `GeneralLp shortest_path_lp(int n, std::span<const WeightedArc> arcs, int s)`：變數是每個頂點到 `s` 的距離 `d(v)`，按講義的形式建模（`WeightedArc{from, to, weight}`）。保證每個頂點從 `s` 都可達。

## Stage 4 — 對偶

`StandardLp dual_of(const StandardLp& lp)`：回傳對偶問題改寫成 standard form 的結果，使得 primal 最大值等於 `dual_of` 的最大值取負（strong duality）。

## 面試追問

1. Equality constraint 拆成兩個 ≤ 之後，解的集合為什麼不變？為什麼拆 free variable 時目標值不會變？
2. 把 max-flow LP 取對偶，每個 dual variable 的意義是什麼？min cut 如何從 dual 的最佳解讀出來？
3. Pivot 只是代數上的等價變形——為什麼它不會改變可行解集合，但會改變目前的 basic solution？
