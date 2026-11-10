---
title: 單一 Consumer 的 Lock-Free Queue（MPSC）
unit: 13 · Data Synchronization（HW10）
source: 6.172 HW10 check-off 8（single dequeuer）
pdf: materials/6.172/homework/hw10/handout.pdf
read: materials/6.172/lectures/lec17.pdf
kind: code
minutes: 60
sanitizer: tsan
---

# 1313 · 單一 Consumer 的 Lock-Free Queue

> **實務上為什麼會遇到**：logging、actor mailbox、event loop 的輸入端幾乎都是多個 producer、一個 consumer。少了「多個 consumer 搶同一個 head」這個需求，dequeue 就能省掉最貴的 CAS。

作業 check-off 8：如果只有一個 thread 會 dequeue，1312 的 lock-free queue 可以怎麼簡化？

沿用 1312 的 tagged word（`_shared/08_node_pool.hpp` 的 `practice::NodePool`，索引加版本計數）。`MpscQueue<Atomic>(capacity)`：只有 tail 和各節點的 `next`、值是 `Atomic`。

## Stage 1 — 序列語意

- `enqueue(value)`：節點用完時回傳 `false`。
- `dequeue()`：空的回傳 `std::nullopt`。**只有一個 thread 會呼叫它**，但可以和多個 `enqueue` 同時進行。

## Stage 2 — 併發

`enqueue` 和 1312 相同（CAS `tail->next`、落後時幫 tail 前進）。`dequeue` 簡化成：

- `head` 是 **consumer 自己的欄位**，不需要 `Atomic`。
- 讀 `head->next`，空的就回傳 `std::nullopt`。
- 沒有別的 thread 會動 head，所以**不需要 CAS head**。

有一個陷阱：釋放舊的 dummy 之前，要確定 `tail` 沒有還指著它（否則 enqueuer 可能還在用這個節點）；所以 tail 落後時 consumer 也要幫忙往前推。

## Stage 3 — 可驗證的簡化

一般情況（沒有 tail 落後）下，`dequeue` **一次 CAS 都不用**：測試會用一個會計數 CAS 的 `Atomic` 來確認。多 producer 的情況下，最終結果和 FIFO 性質由窮舉 interleaving 與真實執行緒的 TSan 測試檢查。

## 面試追問

1. check-off 8 第二問：為什麼這個版本仍然是 non-blocking？一個 producer 卡住時，consumer 看到什麼？
2. 為什麼釋放舊 dummy 前要先確定 tail 沒有落後？少了這一步會出什麼錯？
3. 如果 producer 也只有一個（SPSC），還能拿掉哪些 CAS？ring buffer 的版本長怎樣？
