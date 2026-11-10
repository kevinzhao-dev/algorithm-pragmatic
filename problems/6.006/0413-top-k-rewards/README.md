---
title: Top-K（heap 的三種用法）
unit: 04 · Trees & Heaps（PS4）
source: 6.006 PS4 Problem 4-3
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec08.pdf
kind: code
minutes: 50
---

# 0413 · Top-K

> **實務上為什麼會遇到**：排行榜前 100 名、搜尋結果 re-rank、監控系統的 top-N slow queries，都不需要整份資料排序。差別只在：資料是一次給你，還是源源不絕地來。

一個線上內容平台要在一批內容上發放獎勵。每筆 `Garden<Score>`（內容）有分數 `score` 和編號 `id`。`Score` 只保證有 `operator<`。

建議 `#include "../0405-binary-heap/solution.hpp"`，重用你寫好的 `BinaryHeap`。

## Stage 1 — 一批資料，取前 k 名

`top_k_ids(gardens, k)`：`0 ≤ k ≤ gardens.size()`，回傳分數最高的 `k` 筆的 `id`（同分時任選）。

**O(n + k log n)**：比 `n log n` 的全部排序或逐一 `push` 更省。測試會計比較次數，全部排序會超過上限。

## Stage 2 — 門檻以上的全部

`ids_above(heap, x)`：`heap` 已經是依 `score` 排好的 **max-heap**（陣列形式）。回傳所有 `score > x` 的 `id`（順序不限）。**O(k)**，`k` 是結果數；不能掃過整個 heap。

想想 heap property：一個分數 `≤ x` 的節點，它的整棵子樹都不用看。實際走訪到的節點最多只有答案的兩倍左右。

## Stage 3 — 實務延伸：串流 Top-K Tracker

資料一筆一筆進來、可能永遠不會結束，不能全部存下來。`TopKTracker<T, Compare>`：

- `explicit TopKTracker(k, compare = {})`：只留「最大的 k 個」（依 `compare`）。`k` 可以是 0。
- `offer(v)`：O(log k)，且當 `v` 根本進不了前 k 名時要 O(1) 就放棄（只比一次）。
- `size()`：目前保留幾個，不超過 `k`。
- `threshold()`：留下的 k 個裡最小的那個（要指標，空間滿之前回傳 `nullptr`）；新元素必須大於它才有機會進場。
- `sorted_largest()`：目前保留的元素，由大到小，O(k log k)。

記憶體只能用 O(k)：把東西往大 vector 一塞的做法過不了 `Tracked` 的檢查。

## 面試追問

1. Stage 1：為什麼不一開始就維護一個大小為 k 的 min-heap？跟 build-heap 比，兩者的時間差別在哪裡？
2. Stage 2 為什麼比連續 `pop` 省一個 log？如果 `x` 很小，`k` 接近 `n` 時，哪個做法好？
3. Stage 3：`offer` 從 O(log k) 變 O(1)，為什麼不是加速最多的那種情況？（想想哪種輸入 worst-case）
