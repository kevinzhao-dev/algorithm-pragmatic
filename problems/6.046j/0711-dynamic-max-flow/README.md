---
title: 動態 Max Flow（容量更新不重算）
unit: 07 · Network Flow（PS7）
source: 6.046J PS7 Problem 7-1
pdf: materials/6.046j/psets/ps07/questions.pdf
answer: materials/6.046j/psets/ps07/solutions.pdf
read: materials/6.046j/lectures/lec13.pdf, materials/6.046j/lectures/lec14a.pdf
kind: code
minutes: 150
---

# 0711 · 動態 Max Flow

> **實務上為什麼會遇到**：SDN 控制器的 bandwidth allocation，link 容量隨時被調整（維護、故障、QoS 重分配）；每次重算全網 max flow 代價太大。這題要做到：容量只差 k，工作量也只有 O(k(V+E))。

頂點 `0..n-1`，source、sink 在建構時給定，容量是非負整數。允許平行邊與反向邊，`u == v` 不會出現。

```cpp
class DynamicMaxFlow {
public:
    DynamicMaxFlow(int n, int source, int sink);
    int add_edge(int u, int v, long long capacity);   // edge id 從 0 遞增
    long long compute();                              // 計算最大流，回傳 flow 值
    long long value() const;                          // 目前 flow 值
    long long flow_on(int id) const;
    long long capacity_of(int id) const;
    long long set_capacity(int id, long long r);      // 見下，回傳新的 flow 值
};
```

`add_edge` 都在第一次 `compute` 之前；之後只能 `set_capacity`。

一次只看一關。

## Stage 1 — 建圖與初始 max flow

`compute()` 用 0701 的成果（或重寫）建好 residual graph 並算出一個 max flow。`flow_on` 必須滿足 capacity 和 conservation。

## Stage 2 — 容量增加（Part b）

`set_capacity(id, r)`，`r > c`：回傳新的 max flow。令 `k = r - c`，時間 O(k(V+E))。**不可以**整個重算。

## Stage 3 — 容量減少但流量還放得下（Part c，簡單情況）

`r < c` 而目前 `flow_on(id) ≤ r`：max flow 不會變大，也不需要動到任何流量（為什麼？）。

## Stage 4 — 容量減少而流量放不下（Part c，主要情況）

`r < c` 且 `flow_on(id) > r`：先把這條邊多出來的流量退掉（要恢復 conservation），再試著補回去。令 `k = c - r`，時間 O(k(V+E))。完成後 `flow_on(id) ≤ r` 且仍為 max flow。

## 面試追問

1. 為什麼容量改變 k 時 max flow 至多改變 k？用 min cut 講清楚（Part a）。這跟 O(k(V+E)) 的 k 次 augment 有什麼關係？
2. 退流量時，沿著帶正流量的邊往回走，為什麼一定走得到 s、t 或 v？（conservation invariant；若 flow 圖有環？）
3. 如果容量不是整數，這個 O(k(V+E)) 的論證還成立嗎？怎麼修？
