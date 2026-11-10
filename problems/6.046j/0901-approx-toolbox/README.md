---
title: 近似演算法工具箱（Vertex Cover、Set Cover、Partition、TSP）
unit: 09 · Approximation 與 FPT（PS9）
source: 6.046J Lecture 17, Recitation 9
pdf: materials/6.046j/lectures/lec17.pdf
read: materials/6.046j/lectures/lec17.pdf, materials/6.046j/recitations/rec09.pdf
kind: code
minutes: 130
---

# 0901 · 近似演算法工具箱

> **實務上為什麼會遇到**：NP-hard 問題在真實系統上很少有精確解的餘裕：叢集的機器分配、廣告版位、路由規劃，大家要的是「有保證的夠好」。這題把幾個典型的近似比證明，變成可以驗算的程式。

一次只看一關。測試會在小實例上用暴力解驗證近似比。

## Stage 1 — Vertex Cover 的 2-approximation

`std::vector<int> approx_vertex_cover(int n, std::span<const std::pair<int,int>> edges)`：回傳 cover（頂點升冪，不重複）。演算法：反覆挑一條兩端都還沒被選的邊，把兩個端點都加入 cover。

保證 `|C| ≤ 2·OPT`，O(V+E)。

## Stage 2 — Set Cover 的 greedy

`std::vector<int> greedy_set_cover(int universe, const std::vector<std::vector<int>>& sets)`：反覆選涵蓋最多尚未覆蓋元素的 set，回傳 set 索引（升冪）。輸入保證所有 set 的聯集是 `0..universe-1`。

保證 `|C| ≤ (ln n + 1)·OPT`。複雜度 O(Σ|Sᵢ|·log m)（用 heap 的 lazy 更新，不要每輪重算所有 set）。

## Stage 3 — Partition 的 PTAS

```cpp
struct TwoWaySplit {
    std::vector<int> side_a;      // indices of items in A, ascending
    long long cost = 0;           // max(w(A), w(B))
};
TwoWaySplit approx_partition(std::span<const long long> weights, double eps);
```

把權重分成兩堆，使較重的一堆 ≤ `(1+ε)·OPT`。兩階段：先對最大的 `m = ⌈1/ε⌉ - 1` 個物品暴力求最佳分割，其餘依權重遞減 greedy 放進較輕的一堆。`m` 由 `eps` 決定，`eps ∈ (0, 1]`。輸出的 `side_a` 要是真正的索引集合、`cost` 要與它一致。

## Stage 4 — Metric TSP 的 MST 2-approximation

`std::vector<int> approx_tsp(const std::vector<std::vector<long long>>& dist)`：`dist` 是對稱、滿足三角不等式的距離矩陣，回傳一個從頂點 0 出發的環（頂點序列，長度 n，回到 0 的邊隱含）。依 Recitation 9：MST 的 preorder walk，並把回頭路 short-cut 掉。

保證 tour 成本 ≤ `2·OPT`，O(V²)。

## 面試追問

1. Vertex cover 的 `2·OPT` 上界，為什麼不能用「Max-degree greedy」取代（lecture 的反例）？為什麼 lower bound 要用 matching 來論證？
2. Set cover 的 `ln n` 上界是緊的嗎？除非 P=NP，能不能做得更好？
3. TSP 的 2-approx 需要三角不等式的哪一步？沒有三角不等式時為什麼無法近似（除非 P=NP）？
