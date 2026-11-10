---
title: 動態相鄰的序列壓實（括號式區間 DP）
unit: 08 · Dynamic Programming II（PS8）
source: 6.006 PS8 Problem 8-2, Recitation 17
pdf: materials/6.006/psets/ps8/questions.pdf
answer: materials/6.006/psets/ps8/solutions.pdf
read: materials/6.006/lectures/lec15.pdf, materials/6.006/recitations/rec17.pdf
kind: code
minutes: 95
---

# 0812 · 動態相鄰的序列壓實

> **實務上為什麼會遇到**：LSM-tree 的 compaction、log 的 tombstone 清理、把程式碼裡的暫存變數化掉，常見的型態是「兩筆東西只有在中間被清空之後才能相鄰處理」。這跟 RNA 二級結構和括號配對是同一種形狀：選定第一個元素跟誰配對，序列就裂成內外兩段，彼此獨立。

有一排 record，value 是 `v[i]`（可為負）。可以重複以下操作，任何順序都可以，也可以留下一些 record 不處理：

- **單獨處理** record `i`：得 `v[i]` 分，移除它。
- **配對處理**目前相鄰（夾在中間的都已被移除）的 `i < j`：得 `v[i]·v[j]` 分，兩個都移除。

每筆 record 最多處理一次。這是 Lecture 15 Bowling 的擴充：多出的規則是，中間被清空後，原本不相鄰的兩筆可以配對。

一次只看一關，做完再往下讀。

## Stage 1 — 最高分

`std::int64_t max_compaction_score(std::span<const int> records)`：O(n³) 時間。

子問題要多帶一個布林值。先想清楚「中間那一段一定要清空」和「隨便」差在哪。

## Stage 2 — 全部清空的分數

`std::int64_t max_full_compaction_score(std::span<const int> records)`：每筆 record 都必須被處理的最高分（可能為負）。同樣 O(n³)，是 Stage 1 的狀態順便算出來的東西。

## Stage 3 — 排出操作順序

```cpp
struct Removal { std::size_t first; std::size_t second; };   // first == second: alone; first < second: pair
struct CompactionPlan { std::int64_t score; std::vector<Removal> removals; };
CompactionPlan compaction_plan(std::span<const int> records);
```

`removals` 是**可以依序執行**的操作清單：輪到一個配對時，兩者之間的 record 都必須已經被移除。`score` 是最高分，實際執行清單要得到這個分數。同分時任一組都可以。

## Stage 4 — 實務延伸：晶圓佈線（同一招換個題目）

`n` 個 port 平均分佈在圓周上，`is_source[i]` 表示第 `i` 個是 power source（`true`）或 computing unit（`false`）。要在不交叉的前提下拉出最多條連線，每條連線連接一個 source 和一個 unit，而且不能連在圓周上相鄰的兩個 port（`i` 和 `i+1`，以及 `0` 和 `n−1`）。每個 port 最多一條。

`std::vector<std::pair<std::size_t, std::size_t>> max_noncrossing_wires(std::span<const bool> is_source)`：回傳每條連線 `(a, b)`（`a < b`），O(n³) 時間。

## 面試追問

1. Stage 1 為什麼切成「配對夾住的內部」和「右邊剩下的」兩個獨立問題是對的？在什麼規則下會不獨立？
2. 這個題目是 polynomial、pseudopolynomial 還是 exponential？跟 Stage 4 比，子問題的形狀差在哪？
3. 如果 pair 的條件改成「只要中間沒有更高價值的 record」，這套區間 DP 還成立嗎？
