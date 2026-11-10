---
title: Fixed-Size Block Pool（free list 與 object pool）
unit: 08 · Storage Allocation（HW6）
source: 6.172 Lecture 11 Fixed-Size Allocation；Recitation 6 第 2 題
pdf: materials/6.172/lectures/lec11.pdf
answer: materials/6.172/recitations/r6/solutions.pdf
read: materials/6.172/recitations/r6/problems.pdf
kind: perf
minutes: 100
---

# 0801 · Fixed-Size Block Pool

> **實務上為什麼會遇到**：連線物件、request context、樹節點，數量多、大小一樣、生命週期短。這類物件走通用 allocator 的成本（跨 size class 的查找、每塊 metadata）在熱路徑上很明顯，所以幾乎每個 latency 敏感的系統都有一個 object pool。

把 Lecture 11 的固定大小配置法做成一個真的元件：free list 的 next 指標**直接存在閒置的 block 裡**，所以沒有額外 metadata。

`solution.hpp` 的 `Upstream` 是 chunk 的來源，測試會傳入會數帳的版本。一次只看一關。

## Stage 1 — 單一 chunk 的 free list

`FixedBlockPool(block_size, blocks_per_chunk, upstream)`：

- `block_size()` 是實際每塊大小：`block_size` 往上取到 16 的倍數（要放得下 free list 的指標，且對齊到 16，這樣放任何基本型別都安全）。
- `allocate()`：**優先**拿 free list，否則切 chunk 裡還沒用過的下一塊。第一次需要記憶體時向 upstream 要一個 chunk。平均 O(1)。
- `deallocate(block)`：O(1) 放回 free list。`nullptr` 不做任何事。
- free list 必須是 **LIFO**：最後放回的那塊最先被拿出來。這是刻意的，想想為什麼（面試追問第 1 題）。
- `allocated_blocks()`：目前尚未歸還的 block 數。

本關不會用超過一個 chunk。

## Stage 2 — 成長與歸還

- chunk 用完時向 upstream 再要一個新的，大小一樣，`chunk_count()` 加一。已歸還的 block 用完前不可以要新 chunk。
- 解構子把**所有** chunk 還給 upstream（`Upstream::deallocate` 傳回當初的指標與大小），不管還有沒有 block 在外面。

每個 chunk 只多花常數大小的 metadata：upstream 收到的 chunk 大小不能超過 `blocks_per_chunk * block_size() + 64`。

## Stage 3 — 所有權轉移

補上 move constructor 與 move assignment（`noexcept`）。之前發出去的 block 在 move 之後仍然有效、可以透過新的 owner 歸還；被搬走的 pool 不再持有任何 chunk，且可以被解構。self-move 不能壞掉。

## Stage 4 — 接上 STL：`PoolAllocator<T>`

做一個最小的標準 allocator，讓 node-based 容器（`std::list`、`std::map`）的節點都走 pool：

- 預設建構時建立一份共享的狀態（`std::shared_ptr`）；複製與 rebind 都指向同一份。容器內部會把 `PoolAllocator<T>` rebind 成節點型別，所以共享狀態要能依 `sizeof` 分出不同的 pool。
- `allocate(1)` 走 pool；`allocate(n)`（n ≠ 1）fallback 到 `::operator new`，對應的 `deallocate` 也要走同一條路。
- 兩個 allocator 指向同一份狀態時比較結果為相等。

## 面試追問

1. 為什麼 free list 要用 LIFO？把最久以前歸還的 block 優先發出去（FIFO）會差在哪？對空間使用有差嗎？（Recitation 6 第 2 題）
2. 這個 pool 的 external fragmentation 為什麼幾乎是零？什麼時候它其實比 malloc 浪費？
3. 多執行緒下要把它做成 thread-safe，你會怎麼拆？Lecture 12 的 local heap 跟 local ownership 差在哪？
4. 同樣是配置固定 1040 bytes 的物件，用固定大小 pool 比用 power-of-two binned free lists 好在哪？（Recitation 6 第 3 題）
