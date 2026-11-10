---
title: Michael-Scott Lock-Free Queue
unit: 13 · Data Synchronization（HW10）
source: 6.172 HW10 check-off 3、4、5、6、9（lock-free queue）
pdf: materials/6.172/homework/hw10/handout.pdf
read: materials/6.172/lectures/lec17.pdf
kind: code
minutes: 120
sanitizer: tsan
---

# 1312 · Michael-Scott Lock-Free Queue

> **實務上為什麼會遇到**：JDK 的 `ConcurrentLinkedQueue` 就是這個演算法。它有一個很值得記住的技巧：tail 可以暫時落後，但其他 thread 看到落後時會**幫忙**把它推到正確的位置，這就是 lock-free 的意思：沒有任何一個 thread 能卡住大家。

把作業 Figure 2、3 的演算法寫成 C++。節點由 `_shared/08_node_pool.hpp` 的 `practice::NodePool` 提供，同樣用**索引加版本計數**放進一個 `Atomic<std::uint64_t>`（低 32 位索引、高 32 位計數）：`head`、`tail`、每個節點的 `next` 都是這種 tagged word。作業的 `pointer_t` 就是這個東西。

值是 `std::uint64_t`，存在節點的 `Atomic<std::uint64_t>`（作業第 5(b) 題提到 dequeue 在 CAS **之前**就讀了值，當時節點可能已被別人回收重用，所以值必須是 atomic）。`MsQueue<Atomic>(capacity)`：`capacity` 個元素外加一個 dummy。

## Stage 1 — 序列語意

- `enqueue(value)`：節點用完時回傳 `false`。
- `dequeue()`：空的回傳 `std::nullopt`。

新節點的 `next` 要重設成空，但**保留並增加**舊的版本計數（節點會重複使用）。

## Stage 2 — 併發與 helping（check-off 3、4、5）

寫出作業的完整流程：

- `enqueue`：讀 tail 和 `tail->next`，確認 tail 沒變。`next` 為空就 CAS `tail->next` 接上新節點；否則 tail 落後了，CAS 幫它往前移。成功接上之後，再 CAS 把 tail 移到新節點。
- `dequeue`：讀 head、tail、`head->next`，確認 head 沒變。head 和 tail 相同時：`next` 為空代表空 queue，回傳 `std::nullopt`；否則 tail 落後了，幫忙移動。不同時：**先**讀值，再 CAS head 到 `next`。

因為有 helping，一個 thread 卡在「接上節點、還沒移 tail」時，其他 thread 不會被擋住。測試窮舉這種情形：不可以死鎖、不可以無限重試。

## Stage 3 — ABA（check-off 9）

版本計數的作用：節點被回收重用之後，索引可能跟舊值一樣，但版本不同，所以舊的 CAS 會失敗。測試會窮舉一個 stale `dequeue` 的情境：另一個 thread 在中間把 head 繞回同一個索引。

## Stage 4 — 真的跑

多個 producer 和 consumer：不可以遺失、重複；同一個 producer 的值順序不變。全程要通過 TSan。

## 面試追問

1. check-off 4：為什麼 enqueue 需要兩次 CAS？第二次 CAS 失敗（或 thread 在那之前被殺掉）會發生什麼事？tail 最多能落後多少？
2. check-off 5：為什麼 dequeue 要在 CAS 之前讀值？這個順序反過來會怎樣？
3. check-off 6：lock-free 版本一定比雙鎖快嗎？用 CAS 重試、cache-line 爭奪和競爭程度解釋。
