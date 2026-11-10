---
title: Recurrence 求解器（Master Theorem 與遞迴樹）
unit: 02 · Sorting & Recurrences（PS2）
source: 6.006 PS2 Problem 2-1
pdf: materials/6.006/psets/ps2/questions.pdf
answer: materials/6.006/psets/ps2/solutions.pdf
read: materials/6.006/recitations/rec03.pdf
kind: code
minutes: 60
---

# 0211 · Recurrence 求解器

> **實務上為什麼會遇到**：面試官問「這個分治法複雜度多少」時，你要一眼看出是 leaf-heavy、balanced 還是 root-heavy，才知道該優化哪一層。效能分析裡的 doubling experiment（n 翻倍看時間變幾倍）是同一個概念的量測版本。

原題要你解四個 recurrence，並畫遞迴樹。這題把這個過程做成程式：Master Theorem、遞迴樹的逐層工作量，最後用數值驗證你的答案。**手算原題那四小題（PDF 第 1 頁）放在面試追問。**

`Growth{exponent, log_power}` 代表 `Θ(n^exponent · log^log_power n)`。比較指數時容許誤差 1e-9（`b` 可以是無理數，例如 √2）。

## Stage 1 — Master Theorem

`solve_master(a, b, c, k)` 解 `T(n) = a·T(n/b) + Θ(n^c · log^k n)`，`T(1) = Θ(1)`。回傳 `MasterResult{which, growth}`：

| `which` | 條件（`e = log_b a`） | `growth` |
| --- | --- | --- |
| `MasterCase::Leaves` | `c < e` | `Θ(n^e)` |
| `MasterCase::Balanced` | `c == e` | `Θ(n^c · log^(k+1) n)` |
| `MasterCase::Root` | `c > e` | `Θ(n^c · log^k n)` |

參數需滿足 `a ≥ 1`、`b > 1`、`c ≥ 0`、`k ≥ 0`，否則丟 `std::invalid_argument`。

## Stage 2 — 遞迴樹

- `level_work(a, b, c, k, depth)`：設 `n = b^depth`，回傳遞迴樹每一層的總工作量（`vector<double>`，第 0 層是 root）。第 `i` 層（`i < depth`）有 `a^i` 個子問題，每個大小 `s = b^(depth−i)`，成本 `s^c · (log₂ s)^k`；最後一層 `i == depth` 是 leaves：`a^depth` 個子問題，成本各為 1。
- `tree_total(a, b, c, k, depth)`：所有層的總和。

測試會檢查逐層工作量的型態（逐層變大／持平／變小），並用它驗算 Stage 1：`tree_total` 除以 `Θ(growth)` 的比值在 depth 很大時要趨於穩定。

## Stage 3 — 只有上界的 f(n)

原題的 `f(n)` 寫成 `O(...)` 時，只能斷定上界。`bounds_with_big_o(a, b, c, k)` 解 `T(n) = a·T(n/b) + O(n^c · log^k n)`，回傳 `Bounds{lower, upper}`：

- `upper`：把 `f` 視為 `Θ(n^c log^k n)` 的 Master Theorem 結果。
- `lower`：`f` 可以小到 0，所以是 `Θ(n^e)`（leaves 的貢獻）。
- `tight()`：`lower` 與 `upper` 是否相同（代表 `O` 其實不影響結果）。

## Stage 4 — 減一型 recurrence（chain）

`T(n) = T(n − s) + Θ(n^c · log^k n)`（`s ≥ 1` 是常數）沒有 Master Theorem 可用：

- `solve_chain(c, k)`：回傳 `Growth`，答案是 `Θ(n^(c+1) · log^k n)`。
- `chain_work(s, c, n)`：精確計算 `Σ (n − i·s)^c`（`i` 從 0 開始，加到 `n − i·s ≥ 1` 為止），`c` 是非負整數，回傳 `uint64_t`。測試會確認 n 翻倍時比值符合 `solve_chain` 的答案。

## 面試追問

1. 手算 PDF 上的 2-1 四小題（題目在 PDF 第 1 頁，不要看解答）。
2. Master Theorem 的 Case 3 為什麼需要 regularity condition？對 `f(n) = n^c` 這種多項式為什麼自動成立？
3. 遞迴樹上，每層工作量形成等比數列：公比由什麼決定？公比 > 1、= 1、< 1 分別對應哪個 case，和 Θ 的結果怎麼對應？
