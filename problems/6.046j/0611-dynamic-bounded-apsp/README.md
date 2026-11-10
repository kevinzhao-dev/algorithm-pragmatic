---
title: 動態與限跳數 All-Pairs Shortest Paths
unit: 06 · Augmentation、APSP 與 MST（PS6）
source: 6.046J PS6 Problem 6-1
pdf: materials/6.046j/psets/ps06/questions.pdf
answer: materials/6.046j/psets/ps06/solutions.pdf
read: materials/6.046j/lectures/lec11.pdf
kind: code
minutes: 150
---

# 0611 · 動態與限跳數 APSP

> **實務上為什麼會遇到**：WAN 的 link 權重會隨 congestion 變動，路由引擎不想每次都重算整張表；SLA 又常限制「最多經過 h 個 hop」。這題把靜態 APSP 改造成一個可以線上更新的路由表引擎。

權重皆為非負整數，`w[i][i] == 0`，沒有邊用 `kInf`（標頭已定義）。`i == j` 的更新一律是 no-op。一次只看一關，每關的 class 都獨立，互不干擾。

## Stage 1 — 動態 APSP（Part a）

```cpp
class DynamicApsp {
public:
    explicit DynamicApsp(Matrix w);         // O(V³)
    long long dist(int u, int v) const;     // kInf 表示不可達
    std::vector<int> path(int u, int v) const;     // {u, ..., v}；不可達回傳空；u==v 回傳 {u}
    // 把 w(i,j) 改成 r（r 可為 kInf，代表刪邊）。回傳「距離值真的改變」的 ordered pair 數量。
    long long update(int i, int j, long long r);
};
```

三種情況的複雜度要求不同：

| 情況 | 複雜度 |
| --- | --- |
| `r == w(i,j)` | O(1) |
| `r < w(i,j)`（邊變輕、或新增邊） | O(V²) |
| `r > w(i,j)` | O(V³) 可接受 |

更新之後 `dist` 和 `path` 都必須是新圖上正確的答案。

## Stage 2 — 下界（Part b）

`struct Scenario { Matrix w; int i; int j; long long r; };`

`Scenario worst_update_scenario(int n)`：給定 `n ≥ 2`，建構一張 n 個頂點的圖與一次更新，使任何演算法都得改動 Ω(n²) 個 `D` 的 entry（具體來說：`update` 回傳值至少 `n*n/4`）。想想有多少 ordered pair 的距離會因為一條邊而從 `kInf` 變成有限值。

## Stage 3 — 限跳數的 Floyd-Warshall（Part c）

`Matrix bounded_hop_floyd(const Matrix& w, int h)`：回傳 `D[i][j]` = 最多 h 條邊的 i→j 最短路徑權重，`0 ≤ h ≤ n-1`。時間 O(V³ h²)，空間 O(V² h)。

## Stage 4 — 限跳數的 repeated squaring（Part d）

`Matrix bounded_hop_doubling(const Matrix& w, int h)`：同上定義，時間 O(V³ log h)。

## Stage 5 — 動態限跳數 APSP（Part e）

```cpp
class DynamicBoundedApsp {
public:
    DynamicBoundedApsp(Matrix w, int h);
    long long dist(int u, int v) const;                 // 最多 h 跳
    long long dist_at_most(int u, int v, int k) const;  // 最多 k 跳，0 ≤ k ≤ h
    void update(int i, int j, long long r);
};
```

建構 O(V³ h)。`r == w` 時 O(1)；`r < w` 時 O(V² h²)；`r > w` 時 O(V³ h)。

## 面試追問

1. `r < w(i,j)` 的更新為什麼不需要重算 `d(x,i)` 和 `d(j,y)` 就能安心使用它們？這用到了什麼假設（負邊可以嗎）？
2. 為什麼限跳數時 repeated squaring 比 Floyd-Warshall 好？平常的 APSP 不是剛好相反嗎？
3. `r > w(i,j)` 真的需要 O(V³) 嗎？能不能只重算「最短路徑樹經過這條邊」的 source？最壞情況會省到多少？
