---
title: 資源預約排程（Merge、分治、線上版本）
unit: 02 · Sorting & Recurrences（PS2）
source: 6.006 PS2 Problem 2-5
pdf: materials/6.006/psets/ps2/questions.pdf
answer: materials/6.006/psets/ps2/solutions.pdf
read: materials/6.006/lectures/lec03.pdf
kind: code
minutes: 100
---

# 0215 · 資源預約排程

> **實務上為什麼會遇到**：叢集排程要知道「這批 job 同時要幾台機器」，把一堆區間的需求合併成一條隨時間變化的階梯函數；這跟 merge sort 的合併是同一招。LSM tree 把寫入累積成一層一層再合併，第三關做的就是這件事。

一個 job 要在半開區間 `[start, end)` 占用一台機器（`start < end`）。**排程（schedule）** 是一串 `Reservation{machines, start, end}`，意思是在 `[start, end)` 要 `machines` 台（`machines > 0`）。合法的排程滿足：區間互不重疊、依時間遞增、相鄰且接觸的區間 `machines` 一定不同（相同就該合併成一段）。沒有 job 的時段就是沒有 reservation。一組 job 恰好對應一個合法排程。

例如 jobs `[0,2) [1,3) [2,6)` 的排程是 `(1,[0,1)) (2,[1,3)) (1,[3,6))`。

## Stage 1 — 合併兩個排程

`merge_schedules(a, b)`：`a`、`b` 是兩個合法排程（兩組 job 各自的需求），回傳兩組 job 合起來的排程，**O(|a| + |b|)**。

- 邊界：任一邊是空的、兩邊在同一時間點開始或結束、完全包含、一段 `a` 同時跨過好幾段 `b`、接觸但不重疊（要合併相同機器數的相鄰區間）。
- 建議先想清楚什麼時候「目前時間 `t`」要往下一個事件推進。

## Stage 2 — 分治

`satisfying_schedule(jobs)`：`jobs` 是 `std::span<const JobRequest>`，回傳合法排程，**O(n log n)**，要用 merge sort 的結構（Stage 1 的 merge 當合併步驟）。空的 `jobs` 回傳空排程；`start ≥ end` 的 job 丟 `std::invalid_argument`。

## Stage 3 — 實務延伸：線上累積（LSM 手法）

`IncrementalPlanner`：job 一個一個進來，隨時可能要看目前的排程。

- `add(job)`：攤提 **O(log n)**。不可每次都重算（有 `PERF` 檢查）：維持數個依大小呈倍數成長的小排程（像二進位計數器一樣合併，這正是 LSM tree 的手法）。
- `schedule()`：回傳目前所有 job 的排程，O(n)。
- `peak()`：目前最多同時需要幾台機器，O(n) 即可。
- `size()`：已加入的 job 數。

## 面試追問

1. 為什麼這個問題要用分治而不是把每個端點排序後掃一遍？掃描線法（sort + sweep）的複雜度和常數如何比較？
2. `merge_schedules` 的正確性不變量是什麼？一步驟裡 `t` 可能跳過哪些事件？
3. 把 Stage 3 換成真的 LSM tree：什麼時候觸發 compaction？讀放大和寫放大各是什麼？
