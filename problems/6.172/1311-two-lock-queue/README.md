---
title: 雙鎖 FIFO Queue（head 鎖與 tail 鎖）
unit: 13 · Data Synchronization（HW10）
source: 6.172 HW10 check-off 1、2、7（two-lock queue）
pdf: materials/6.172/homework/hw10/handout.pdf
read: materials/6.172/lectures/lec17.pdf
kind: code
minutes: 80
sanitizer: tsan
---

# 1311 · 雙鎖 FIFO Queue

> **實務上為什麼會遇到**：工作佇列的 producer 和 consumer 通常在不同的 thread 上，一把鎖會讓它們互相等待。把鎖拆成 head 和 tail 之後，兩端可以同時動作，這幾乎是所有 bounded queue 的起點。

Michael & Scott 的雙鎖 queue：用 dummy node 讓 head 和 tail 永遠指向不同（或同一個空）節點，`enqueue` 只碰 tail，`dequeue` 只碰 head。

節點由 `_shared/08_node_pool.hpp` 的 `practice::NodePool` 提供，初始化時預先配置（作業的 `new_node` / `free_node`）。模板參數 `Mutex`（預設 `std::mutex`）、`Atomic`（預設 `std::atomic`）用來換掉同步機制：測試會換成 `dsched::Mutex` / `dsched::Atomic` 窮舉 interleaving，也用真的鎖跑 TSan。

## Stage 1 — 序列語意（check-off 1）

`TwoLockQueue<T, Mutex, Atomic>(capacity)`：最多同時放 `capacity` 個元素（pool 要多一個節點給 dummy）。

- `enqueue(value)`：回傳是否成功，節點用完時回傳 `false`。
- `dequeue()`：回傳 `std::optional<T>`，空的回傳 `std::nullopt`。

`T` 可能是 move-only（例如 `std::unique_ptr`），不可以要求可複製。

## Stage 2 — 兩把鎖（check-off 2）

`enqueue` 只拿 tail 鎖，`dequeue` 只拿 head 鎖，兩邊可以同時在各自的臨界區內。

**注意**：queue 空的時候，tail 指到的節點和 head 指到的節點是同一個，enqueuer 寫它的 `next`，dequeuer 讀它的 `next`，這是真正的競爭，所以 `next` 必須是 `Atomic`。作業的 pseudocode 把它當普通指標，是因為那裡假設寫入是 atomic 的；在 C++ 裡一定要明講。

FIFO 的約束（check-off 1）：同一個 producer 依序 enqueue 的值，dequeue 的順序也要一樣；每個值恰好被取出一次。

## Stage 3 — 只有一個 enqueuer（check-off 7）

`SingleEnqueuerQueue<T, Mutex, Atomic>(capacity)`：保證只有一個 thread 會呼叫 `enqueue`，所以可以把 tail 鎖拿掉，`enqueue` 完全不拿鎖。`dequeue` 不變（仍可能有多個 consumer）。

## 面試追問

1. 為什麼需要 dummy node？拿掉 dummy，空 queue 的情況會怎麼出問題？（作業 check-off 3 前的 Figure 1）
2. check-off 2：雙鎖版本的 throughput 上限在哪？什麼 workload 下跟一把鎖差不多？
3. Stage 3 裡 enqueuer 為什麼可以完全不拿鎖？如果也只有一個 consumer，`dequeue` 的 head 鎖還需要嗎？
