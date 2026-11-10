---
title: Lock-Free Stack 與 ABA 問題
unit: 13 · Data Synchronization（HW10）
source: 6.172 Lecture 17 Lock-Free Stack、ABA
pdf: materials/6.172/lectures/lec17.pdf
kind: code
minutes: 90
sanitizer: tsan
---

# 1302 · Lock-Free Stack 與 ABA 問題

> **實務上為什麼會遇到**：配置器的 free list、task pool 都長這樣：一個 CAS 換掉 head。看起來最簡單的 lock-free 結構，一旦節點會被重複使用，就會踩到 ABA：head 的值跟你讀到時一樣，但世界已經變了。

一個由預先配置的節點組成的堆疊。節點由 `_shared/08_node_pool.hpp` 的 `practice::NodePool` 提供（`allocate()` 回傳索引，用完 `kNilIndex`；`release(i)` 歸還；先歸還的先被重用，所以節點會很快回到另一個位置）。節點用**索引**而不是指標，這樣索引和一個版本計數可以放進同一個 64-bit word，用一次 CAS 一起更新。

所有共享欄位用模板參數 `Atomic` 宣告（預設 `std::atomic`）。測試會換成 `dsched::Atomic` 窮舉 interleaving，也會用真的 atomic 跑 TSan。

`LockFreeStack<Atomic>(capacity)`：值是 `std::uint64_t`。

## Stage 1 — 序列語意

- `push(value)`：回傳是否成功（節點用完時回傳 `false`）。
- `pop()`：回傳 `std::optional<std::uint64_t>`，空的回傳 `std::nullopt`。

## Stage 2 — 併發

把 `head` 改成一個 `Atomic<std::uint64_t>`：`push` 和 `pop` 都用 CAS 換掉 head，失敗就重試。節點的 `next` 要用 `Atomic`（另一個 thread 可能正在讀一個已被搬走的節點）。`value` 是 push 時寫、pop 成功後讀，可以是普通欄位，但兩邊要建立 happens-before。

測試窮舉幾個小情境：每個被 push 的值，最後要恰好被 pop 一次（或留在堆疊裡）。

## Stage 3 — ABA

head 的 64-bit 裡，低 32 位放索引、高 32 位放**版本計數**，每次成功的 CAS 都讓版本加一。

測試會窮舉這個情境（pool 只有 2 個節點）：thread A 開始 `pop`，在讀完 head 和 `next`、CAS 之前被搶走；thread B 把兩個節點都 pop 掉，再 push 一個新值，重複使用的節點剛好就是舊的 head，但它的 `next` 已經不同。沒有版本計數時，A 的 CAS 會成功，把一個已經回收的節點接回去。

建議先把版本計數拿掉跑一次，親眼看測試抓到它。

## 面試追問

1. 除了版本計數，還有哪些解 ABA 的方法？hazard pointer、epoch-based reclamation 的取捨是什麼？
2. 32 位的版本計數真的不會繞回來嗎？什麼情況下要擔心？
3. 為什麼這個堆疊是 lock-free 卻不是 wait-free？在高競爭下 pop 重試的成本會怎麼變化？
