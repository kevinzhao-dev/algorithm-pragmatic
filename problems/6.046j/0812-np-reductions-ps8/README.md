---
title: NP 完全性證明寫成程式（Triple-SAT、DONUT、SCHED）
unit: 08 · LP 與 NP-Completeness（PS8）
source: 6.046J PS8 Problem 8-2
pdf: materials/6.046j/psets/ps08/questions.pdf
answer: materials/6.046j/psets/ps08/solutions.pdf
read: materials/6.046j/lectures/lec16.pdf, materials/6.046j/recitations/rec08.pdf
kind: code
minutes: 160
---

# 0812 · NP 完全性證明寫成程式

> **實務上為什麼會遇到**：當 PM 問「這個排程能不能保證最賺錢」，答案常常是：判定版本是 NP-complete，但 pseudo-polynomial DP 在真實數字範圍內就夠快。把證明寫成程式，兩者的界線才清楚。

SAT 公式用 `Cnf{num_vars, clauses}`：變數 `1..num_vars`，`clauses[i]` 是 literal（正負整數，`-3` 表示 `¬x3`）列表。賦值 `Assignment` 是 `std::vector<bool>`，長度 `num_vars + 1`，索引 0 不用。

一次只看一關，測試用暴力解在小實例上檢查 reduction 兩方向。

## Stage 1 — TRIPLE-SAT（Part a）

`bool satisfies(const Cnf& f, const Assignment& a)`：SAT 的 verifier（NP membership）。

`Cnf reduce_sat_to_triple_sat(const Cnf& f)`：回傳 `f'`，使得 `f` 可滿足 ⟺ `f'` 至少有三組**不同**的滿足賦值。`Assignment lift_triple_sat_to_sat(const Cnf& f, const Assignment& a)`：把 `f'` 的滿足賦值還原成 `f` 的滿足賦值。

## Stage 2 — DONUT（Part b）

3SAT 公式保證每個 clause 恰好 3 個 literal（變數可能重複出現）。

```cpp
struct DonutInstance {
    int n = 0;                                   // vertices 0..n-1
    std::vector<std::pair<int,int>> edges;
    std::vector<long long> profit;               // nonnegative
    long long k = 0;
};
DonutInstance reduce_3sat_to_donut(const Cnf& f);
Assignment lift_donut_to_3sat(const Cnf& f, const DonutInstance& inst, std::span<const int> chosen);
```

`chosen` 是互不相鄰、總 profit ≥ k 的頂點集合；回傳 `f` 的滿足賦值。

## Stage 3 — SCHED（Part c）

工作 `Task{time, profit, deadline}`，單機、不可中斷；排程 `Schedule` 是 `(task, start)` 的序列（順序不限，start 為非負整數）。

- `bool valid_schedule(std::span<const Task> tasks, const Schedule& schedule)`：工作不重複、彼此不重疊、每個工作都在 deadline 之前結束（`start + time ≤ deadline`）。
- `long long schedule_profit(tasks, schedule)`：總 profit。
- 判定問題 SCHED：是否存在 valid schedule，profit ≥ k。`SubsetSumInstance{a, t}` 的 reduction：`SchedInstance reduce_subset_sum_to_sched(const SubsetSumInstance&)`，`SchedInstance{tasks, k}`。
- `std::vector<int> lift_sched_to_subset_sum(src, dst, schedule)`：把 schedule 還原成 index 子集，和為 `t`。

## Stage 4 — 實務延伸：pseudo-polynomial 最佳排程

SCHED 是 weakly NP-hard，代表數字不大時有快速的精確解。`Schedule best_schedule(std::span<const Task> tasks)`：回傳 profit 最大的 valid schedule（全部 deadline 之和不大），O(n log n + n·D)，D = 最大 deadline。提示：EDF 排序後做 knapsack。

## 面試追問

1. Stage 1 的 reduction 只加了兩個變數就成立，為什麼？如果題目要求「恰好三組」呢？
2. 為什麼 DONUT 的最佳化版本是 NP-hard 表示「求最佳解」的多項式演算法會推出 P = NP？（decision 與 optimization 的關係。）
3. Stage 4 的 DP 為什麼不證明 P = NP？和 weak / strong NP-hardness 的關係？
