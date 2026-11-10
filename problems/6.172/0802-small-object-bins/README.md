---
title: Small-Object Bins（size class 配置器）
unit: 08 · Storage Allocation（HW6）
source: 6.172 Lecture 11 Variable-Size Allocation（binned free lists）
pdf: materials/6.172/lectures/lec11.pdf
kind: perf
minutes: 90
---

# 0802 · Small-Object Bins

> **實務上為什麼會遇到**：tcmalloc、jemalloc、mimalloc 的快路徑都是這個結構：把請求大小 round up 到某個 size class，每個 class 一條 free list。理解它，才講得清楚為什麼「配置 33 bytes 其實拿到 48」、以及為什麼 sized delete 能省下 header。

用 power-of-two 的 bin 做一個小物件配置器：16、32、…、4096 bytes 共 9 個 bin，每個 bin 一條 free list，整個 chunk 只切給一個 bin 用。大於 4096 bytes 的請求直接交給 `::operator new`。

## Stage 1 — size class

`bin_index(size)`：回傳能放下 `size` 的最小 bin（16-byte bin 是 0，4096-byte bin 是 8）。`size` 的範圍是 1 到 4096。`bin_size(index)` 回傳該 bin 的 block 大小。兩個都要是 O(1)。

## Stage 2 — 配置與歸還

- `allocate(size)`（1 ≤ size ≤ 4096）：從對應 bin 的 free list 拿；沒有就從這個 bin 的目前 chunk 切一塊；chunk 也用完就向 upstream 要新的，大小為建構子給的 `chunk_bytes`。
- `deallocate(p, size)`：`size` 必須與當初 `allocate` 的引數相同，因此 block 上**不需要 header**。O(1) 放回對應 bin。
- 回傳的指標對齊到 16。每個 bin 的 free list 都是 LIFO。

不同 bin 的 block 絕對不能互相重疊，同一個 bin 的 block 歸還後不可以被拿去服務別的 bin。

## Stage 3 — 大請求與邊界

- `size > 4096`：allocate 走 `::operator new`，deallocate（依 size 判斷）走 `::operator delete`，`large_allocations()` 回傳曾經走這條路的次數。
- `size == 0`：當作 1 byte 處理，回傳的指標可以正常 deallocate，對應的 `deallocate(p, 0)` 不可以出錯。
- 解構子把所有 chunk 還給 upstream。
- `live_bytes()`：所有尚未歸還的請求，**使用者要求的 bytes**（不是 block 大小）總和。

## Stage 4 — 量測

沒有新介面。`PERF` 會用同一個執行檔，拿混合大小的小物件工作負載，比較你的配置器和 `::operator new/delete`，要快 2 倍以上（僅 Release）。

## 面試追問

1. Stage 3 的 `live_bytes()` 與 upstream 給出去的 bytes 差多少才算「浪費」？internal fragmentation 的上界是多少？用 power-of-two bin 是最好的選擇嗎？
2. Lecture 11 分析說，binned free list 可能用到 O(M log M) 的虛擬記憶體，為什麼？跟 external fragmentation 的關係是什麼？
3. 為什麼 C++14 的 sized deallocation 對這種配置器是加分？如果呼叫端傳錯 size，會在什麼時候、以什麼形式出事？
