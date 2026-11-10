---
title: Interval Scheduling（Greedy 與 Weighted DP）
unit: 01 · Scheduling 與 Divide & Conquer（PS1）
source: 6.046J Lecture 1, Recitation 1
pdf: materials/6.046j/lectures/lec01.pdf
read: materials/6.046j/lectures/lec01.pdf, materials/6.046j/recitations/rec01.pdf
kind: code
minutes: 75
---

# 0101 · Interval Scheduling

> **實務上為什麼會遇到**：GPU 叢集、會議室、廣告版位的預約系統，都要回答「單一資源上最多接幾筆單」或「接哪些單營收最高」。只差一個權重，能用的演算法就從 greedy 換成 DP。

每筆請求佔用半開區間 `[start, finish)`，`value` 是接下這筆的收益。兩筆請求相容的條件是 `a.finish <= b.start` 或 `b.finish <= a.start`，所以只有端點相碰的兩筆可以同時接。

合法輸入是 `start < finish` 且 `value >= 0`，否則丟 `std::invalid_argument`。所有回傳的請求編號都依開始時間遞增。最佳解不唯一時，回傳任何一組都算對。

## Stage 1 — 最多接幾筆

`max_compatible(requests)`：回傳數量最多的相容子集，忽略 `value`。O(n log n)。

Lecture 1 列了四條 greedy 規則，只有一條是對的。先想清楚另外三條的反例再動手，測試會全部驗。

## Stage 2 — 收益最高

`max_value_schedule(requests)`：回傳總收益最高的相容子集，`total` 是收益總和，`chosen` 是選到的請求。O(n log n)，所以對每筆請求找「前一個相容的位置」時要用二分搜，不是線性掃。

總收益會用 `int64` 存，輸入保證不會溢位。

## Stage 3 — 實務延伸：接單的機會成本

業務問：「這筆 VIP 單一定要接的話，總收益會變多少？」做一個 `WhatIfPlanner`：

| 操作 | 行為 | 複雜度 |
| --- | --- | --- |
| `WhatIfPlanner(requests)` | 預處理；建構完之後不得再依賴傳入的 `span` | O(n log n) |
| `best_total()` | 不強制接任何單時的最高收益 | O(1) |
| `best_with(i)` | 強制接下第 `i` 筆時的最高收益；`i` 超出範圍時丟 `std::out_of_range` | O(log n) |
| `regret(i)` | `best_total() - best_with(i)` | O(log n) |

## 面試追問

1. 用 exchange argument 說明為什麼 earliest finish time 的 greedy 是最佳的，另外三條規則各舉一個反例。
2. Weighted 版本為什麼 greedy 就不行了？把 DP 的子問題定義和轉移講清楚，並說明回溯時為什麼選到的區間自動依開始時間排好。
3. 如果資源從一台變成 `m` 台不同機型，每筆請求只能跑在部分機型上，這題會變成什麼複雜度類別？實務上你會怎麼處理？
