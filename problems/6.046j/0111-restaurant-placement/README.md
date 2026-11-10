---
title: Tree Placement（反例、Tree DP、Greedy、NP-hard）
unit: 01 · Scheduling 與 Divide & Conquer（PS1）
source: 6.046J PS1 Problem 1-1
pdf: materials/6.046j/psets/ps01/questions.pdf
answer: materials/6.046j/psets/ps01/solutions.pdf
read: materials/6.046j/lectures/lec01.pdf
kind: code
minutes: 120
---

# 0111 · Tree Placement

> **實務上為什麼會遇到**：在一棵由網路或組織階層形成的樹上佈署服務，相鄰節點共用同一個 failure domain，不能同時放。每個位置有不同的收益，要挑哪些位置放？同樣的結構也會出現在排程排除和候選人去重。

`Network` 是無向圖，頂點編號 `0..n-1`，`value[v] ≥ 0` 是頂點的收益，`edges` 列出所有邊。**相鄰的兩個頂點不能同時選**。每個函式都回傳**遞增排序**的頂點編號；最佳解不唯一時回傳任一組。

## Stage 1 — Greedy 為什麼不對

- `greedy_by_value(network)`：不斷選收益最高的頂點（同收益選編號小的），把它和所有鄰居移除，直到沒有頂點。任意圖都要能跑，O((n+m) log n)。
- `greedy_counterexample()`：回傳一棵**不超過 12 個頂點的樹**，讓上面的 greedy **得不到**最大收益。

## Stage 2 — Tree DP

`max_value_placement(tree)`：`tree` 保證是樹（空圖也算）。回傳總收益最大的合法選擇，**O(n)**。

根由你自己選。輸入可能是一百萬個頂點的一條長鏈，遞迴會把 stack 撐爆，要用迭代。

## Stage 3 — 只算數量

`max_count_placement(tree)`：每個位置收益都一樣，只要**數量最多**。不准使用 `value`，要用一個 O(n) 的 greedy，不是把 Stage 2 套上去。

輸入規模與 Stage 2 相同。

## Stage 4 — 任意圖

`max_value_placement_general(network)`：圖不一定是樹，`n ≤ 40`，收益最大的合法選擇。

這在一般圖上是 NP-hard 的 maximum weight independent set，所以不用找多項式時間解。枚舉 2^n 種子集在時限內跑不完，請想個更有效率的搜尋方式。

## 面試追問

1. Tree DP 的狀態是什麼？回溯時為什麼要從根往下做，不能逐點獨立判斷？
2. Stage 3 的 greedy 為什麼是對的？（提示：存在最佳解包含所有葉子的論證。）帶權重時為什麼壞掉？
3. 如果一般圖的問題要在 `n = 10^5` 上「夠好就行」，你會怎麼做，如何判斷解的品質？
