---
title: 守恆的狀態機（每個迴圈的點數差都必須為 0）
unit: 06 · Weighted Shortest Paths（PS6）
source: 6.006 PS6 Problem 6-4
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec12.pdf, materials/6.006/lectures/lec14.pdf
kind: code
minutes: 90
---

# 0614 · 守恆的狀態機

> **實務上為什麼會遇到**：計費系統、遊戲經濟、配額管理都是狀態機加上點數增減。只要有一個可以無限繞的迴圈會賺點數（或虧點數），系統遲早會被玩壞；上線前可以用靜態分析先擋下來。

一個狀態機有 `states` 個狀態（`0 … states-1`），每個轉移都會改變使用者的點數：

```cpp
struct Transition { int from; int to; std::int64_t delta; };
```

系統是**守恆**的，代表「從任一個狀態出發，繞任意一圈回到原狀態，點數的總變化都是 0」（只看沿著轉移方向走得通的圈；轉移是單向的，`a → b` 不代表有 `b → a`）。轉移可以是 self-loop，也可以有多條平行轉移。

狀態機不一定連通。可能有上百萬個狀態，**不可以遞迴**。一次只看一關，做完再往下讀。

## Stage 1 — 判斷

`is_conservative(states, transitions)`：回傳系統是否守恆。

要 O(states · |transitions|)，`states` 到幾千時要撐得住。

## Stage 2 — 找出違規的迴圈

`find_unbalanced_cycle(states, transitions)`：系統不守恆時，回傳一個點數總變化不為 0 的**簡單**迴圈，以 `transitions` 的**索引**依序列出（第 `i` 個的 `to` 是第 `i+1` 個的 `from`，最後一個接回第一個，每個狀態最多經過一次）。守恆時回傳 `std::nullopt`。

要 O(states · |transitions|)。

## Stage 3 — 線性時間

把 `is_conservative` 改成 **O(states + |transitions|)**。這題的資料量（幾百萬）用 Bellman-Ford 跑不完。

（Stage 2 的 `find_unbalanced_cycle` 不用變快。）

## 面試追問

1. 為什麼只要檢查「沒有負環」和「沒有正環」就足夠？跟守恆的定義怎麼對應？
2. Stage 3 為什麼可以用 potential 的想法？為什麼不能直接對整張圖指派 potential？（`a → b` 和 `c → b` 兩條邊沒有環，但 potential 可能衝突。）
3. 如果現在要回傳的是「守恆」的憑證，一個正確的 potential 函數就是憑證：驗證它要多少時間？
