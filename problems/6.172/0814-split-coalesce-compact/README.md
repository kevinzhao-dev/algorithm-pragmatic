---
title: 切分、合併與 Compaction（兩種尺寸的 heap）
unit: 08 · Storage Allocation（HW6）
source: 6.172 HW6 §6 Write-up 7–8（split／coalesce 兩種尺寸）
pdf: materials/6.172/homework/hw06/handout.pdf
read: materials/6.172/lectures/lec11.pdf
kind: code
minutes: 80
---

# 0814 · 切分、合併與 Compaction

> **實務上為什麼會遇到**：配置器的 free list 能不能把散落的小塊併回大塊，決定了長時間跑下來會不會「明明還有記憶體卻配置失敗」。一旦物件可以被搬動，就能做 compaction，這正是 copying GC 和 Go 調整 stack 大小的基礎。

沿用上一題的兩種尺寸：**large = 64 bytes**、**small = 32 bytes**。`TwoSizeHeap` 管理一段 `lines` 條 64-byte line 的 arena，只用 offset，不碰真實記憶體。一開始所有 line 都在 large 的 free list 上。

## Stage 1 — 切分，不合併（Write-up 7 前半）

`TwoSizeHeap heap(lines, Coalesce::Never)`：

- `alloc_large()`：取一個閒置的 line，回傳 offset（64 的倍數）。沒有就回傳 `std::nullopt`。
- `alloc_small()`：先用 small 的 free list；沒有的話拿一個閒置的 line **切半**，回傳低位那半，高位那半放進 small 的 free list。都沒有就回傳 `std::nullopt`。
- `free_small(offset)` / `free_large(offset)`：放回對應的 free list，**不做任何合併**。
- `free_bytes()`：目前閒置的總 bytes。

你會在測試裡親眼看到作業問的現象：一直切而不合併時，`free_bytes` 還有一大堆，large 卻配不出來。

## Stage 2 — 合併相鄰的一半（Write-up 7 後半）

`Coalesce::Adjacent` 模式：`free_small` 時如果同一條 line 的另一半也閒置，兩半立刻合併回閒置的 large，`alloc_large` 就能再用它。其餘行為相同，`Coalesce::Never` 模式的測試要繼續通過。

## Stage 3 — 不相鄰的兩半：用 handle 搬動物件（Write-up 8）

兩個不相鄰的 32-byte 區塊沒辦法直接湊成 64 bytes，除非可以搬動物件。`CompactingHeap` 用 handle 間接指向物件，物件的真實位置可以改變：

- `alloc_small()` / `alloc_large()` 回傳 `std::optional<Handle>`。
- `deref(handle)` 回傳該物件目前的 `std::span<std::byte>`（small 32 bytes、large 64 bytes）。先前拿到的 span 在下一次會移動物件的操作後失效。
- `free(handle)`。
- `compact()`：把「只有一半被用的 line」兩兩配對，把一條 line 上的物件搬到另一條的空位，讓盡量多的 line 完整閒置。物件內容不可改變，handle 不可失效。完成後最多只有一條 line 是半滿的。
- `alloc_large()` 在沒有整條閒置 line、但整體閒置空間足夠時，要自動 `compact()` 後重試。

## 面試追問

1. 只切不合併，最壞情況下記憶體會浪費到什麼程度？真實的 buddy 系統（0803）為什麼只合併 buddy，不合併任意兩塊相鄰的？
2. 為什麼 C/C++ 的 `malloc` 不能像 `CompactingHeap` 這樣搬動物件？哪些語言或場景可以？代價是什麼？
3. handle 多一層間接，每次存取都多一次記憶體 load。什麼情況下這個成本值得？（提示：Lecture 11 的 copying GC）
