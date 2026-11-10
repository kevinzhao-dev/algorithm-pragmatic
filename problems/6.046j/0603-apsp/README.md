---
title: All-Pairs Shortest Paths 四件套
unit: 06 · Augmentation、APSP 與 MST（PS6）
source: 6.046J Lecture 11
pdf: materials/6.046j/lectures/lec11.pdf
read: materials/6.046j/lectures/lec11.pdf
kind: code
minutes: 130
---

# 0603 · All-Pairs Shortest Paths 四件套

> **實務上為什麼會遇到**：路由表預先計算、推薦系統的 graph-distance 特徵、排程系統的 min/max delay 限制，全部都會落在這幾種 APSP 或它們的變形上。實務上選哪一個，取決於圖是稠密還是稀疏。

權重可為負，但 `w[i][i] == 0`。沒有邊用 `kInf` 表示（標頭已定義）。**負環**（negative cycle）要回報，不是崩潰。頂點編號 `0..n-1`。

一次只看一關。

## Stage 1 — Min-plus 矩陣與 repeated squaring

- `Matrix min_plus(const Matrix& a, const Matrix& b)`：`(a ⊗ b)[i][j] = min_k (a[i][k] + b[k][j])`，遇到 `kInf` 要正確處理。
- `std::optional<Matrix> apsp_repeated_squaring(const Matrix& w)`：回傳距離矩陣；有負環回傳 `std::nullopt`。時間 O(n³ log n)。

## Stage 2 — Floyd–Warshall 與路徑還原

```cpp
class FloydWarshall {
public:
    explicit FloydWarshall(Matrix w);      // O(n³)
    bool has_negative_cycle() const;
    long long dist(int u, int v) const;    // kInf 表示不可達（無負環時才保證意義）
    std::vector<int> path(int u, int v) const;  // {u, ..., v}；不可達或有負環時回傳空；u==v 時是 {u}
};
```

## Stage 3 — Johnson（稀疏圖）

`std::optional<Matrix> johnson(int n, std::span<const Edge> edges)`，`Edge{from, to, weight}`。用 Bellman–Ford 算 potential 再跑 V 次 Dijkstra。時間 O(VE + V² log V)。有負環回傳 `std::nullopt`。

## Stage 4 — 實務延伸：差分約束排程

工作流系統常見「B 要在 A 結束後 5 到 10 分鐘內開始」這類限制。每條限制寫成 `Constraint{later, earlier, bound}`，意思是 `x[later] - x[earlier] ≤ bound`（x 為整數，可負）。

`std::optional<std::vector<long long>> solve_difference_constraints(int n, std::span<const Constraint> constraints)`：回傳一組滿足全部限制的 `x`，無解時回傳 `std::nullopt`。時間 O(VE)。

## 面試追問

1. Matrix multiplication 版本為什麼不能用 Strassen 加速？min-plus semiring 缺什麼？
2. Johnson 的 reweighting 為什麼能保留所有最短路徑？除了 Bellman–Ford，h 還需要滿足什麼條件？
3. 差分約束無解等價於圖上有負環——用一句話講清楚這個 contradiction 的來源（對環上的不等式求和）。
