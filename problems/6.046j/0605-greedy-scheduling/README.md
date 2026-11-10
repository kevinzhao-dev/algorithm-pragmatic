---
title: 貪婪排程（SPT、SRPT、Interval Partitioning）
unit: 06 · Augmentation、APSP 與 MST（PS6）
source: 6.046J Recitation 6
pdf: materials/6.046j/recitations/rec06.pdf
read: materials/6.046j/recitations/rec06.pdf
kind: code
minutes: 75
---

# 0605 · 貪婪排程

> **實務上為什麼會遇到**：batch job 排程、CPU 排程的 SJF/SRPT、會議室與 GPU 機台的最少配置數。面試常見的 "meeting rooms II" 就是最後一關。

一次只看一關。所有時間都是整數。

## Stage 1 — Shortest Processing Time First

`std::vector<int> spt_order(std::span<const long long> durations)`：回傳一個執行順序（工作索引），使單機上的 total completion time 最小。O(n log n)。

`long long total_completion_time(std::span<const long long> durations, std::span<const int> order)`：依該順序計算 `Σ C_i`，時間從 0 開始、無閒置。

## Stage 2 — 線上版本：SRPT

`Job{release, duration}`：工作在 `release` 時才知道。允許搶占（preemption），每個時刻一台機器只跑一個工作，可以在任何整數時間切換，切換無成本。

`std::vector<long long> srpt_completion(std::span<const Job> jobs)`：回傳每個工作的完成時間，使 `Σ (完成時間)` 最小。O(n log n)。

邊界：同時到達、工作在別的工作跑到一半時到達、duration 相同。

## Stage 3 — Interval partitioning（實務延伸：最少機台數）

`Interval{start, finish}`，half-open：`[start, finish)`，保證 `start < finish`。要把所有區間分配給機台，同一台機台上的區間互不重疊。

```cpp
struct Allocation {
    int machines = 0;              // minimum possible
    std::vector<int> assignment;   // machine id per interval, 0..machines-1
};
Allocation allocate(std::span<const Interval> intervals);
```

O(n log n)。

## 面試追問

1. SPT 的 exchange argument 怎麼寫？如果工作有權重（weighted completion time），排序鍵要換成什麼？
2. 為什麼「依開始時間排序 + min-heap」能達到 maximum overlap 這個 lower bound？如果改成依結束時間排序會發生什麼？
3. 為什麼可以熔化的 fractional make-change 用貪婪是對的，但整數硬幣的 make-change 不行？舉一組反例面額。
