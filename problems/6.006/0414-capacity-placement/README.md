---
title: 容量排程（heap＋hash 的組合）
unit: 04 · Trees & Heaps（PS4）
source: 6.006 PS4 Problem 4-4
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec08.pdf, materials/6.006/recitations/rec08.pdf
kind: code
minutes: 75
---

# 0414 · 容量排程

> **實務上為什麼會遇到**：Kubernetes 的 scheduler、資料庫的 connection pool 分配、CDN 的流量排程，本質都是「把需求塞到還有容量的機器上」。單一資料結構解決不了，真正的工程是把 heap、hash table 和指標關係接對。

叢集有 `n` 台機器，每台有固定容量。任務（用唯一的字串名稱識別）有資源需求 `demand`，放到一台機器上要滿足：同一台機器上的 demand 總和不超過容量。

建議 `#include "../0406-updatable-heap/solution.hpp"`，重用可更新優先權的 heap；再加上 hash table。

`CapacityPlanner` 的介面：

## Stage 1 — 放置任務

- `explicit CapacityPlanner(machines)`：O(n)。
- `place(task, demand)`：把任務放到**任何一台** `available ≥ demand` 的機器上，回傳它的 id；放不下時回傳 `std::nullopt` 且不改變狀態。期望 O(log n)。`task` 已存在時丟 `std::invalid_argument`。
- `available(machine)`：這台機器還剩多少容量，期望 O(1)；未知 id 丟 `std::out_of_range`。

`available == demand` 是放得下的。

## Stage 2 — 釋放

`release(task)`：把任務從機器上移除，把容量還回去。期望 O(log n)。未知的 task 丟 `std::out_of_range`。任務釋放後，它的名字可以再次使用。

要點：只有握著「這台機器在 heap 裡的位置」，才能在 O(log n) 內更新它。這是你剛做好的 handle 派上用場的時候。

## Stage 3 — 查詢

`tasks_on(machine)`：這台機器上所有任務的名稱（順序不限），O(k)，`k` 是回傳數量。未知的 machine 丟 `std::out_of_range`。

## Stage 4 — 實務延伸：Node Drain

維護某台機器前要先把任務搬走：

`drain(machine)`：把這台機器上的每個任務搬到**其他機器**（每個任務都要有地方放，你可以用任何放置順序；搬回自己身上是不合格的）。全部搬完回傳 `true`，此後這台機器沒有任務，且可以再被放置。只要有任何一個任務搬不走，就回傳 `false`，且**整個狀態要完全恢復原狀**（每個任務都回到原機器，各機器的 `available` 都一樣）。未知 machine 丟 `std::out_of_range`。機器上沒有任務時回傳 `true`。

（`drain` 的成敗可能跟放置順序有關，這是裝箱問題的本質，測試只檢查沒有爭議的情況。）

## 面試追問

1. 如果 `demand` 很大，`place` 為什麼只需要看「最空」的那台機器就夠判斷放不放得下？
2. 這裡用的 heap 和 hash 有哪些操作是**期望**、哪些是**最差**？如果面試官要求全都最差 O(log n)，要怎麼換？
3. Drain 的回滾要記下什麼資料？失敗時的時間複雜度是多少？
