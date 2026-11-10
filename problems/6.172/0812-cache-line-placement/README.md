---
title: Cache Line 對齊 vs 緊密排列（配置策略實驗）
unit: 08 · Storage Allocation（HW6）
source: 6.172 HW6 §5–§6 Write-up 1–3（cache-friendly allocation）
pdf: materials/6.172/homework/hw06/handout.pdf
read: materials/6.172/lectures/lec11.pdf
kind: code
minutes: 80
---

# 0812 · Cache Line 對齊 vs 緊密排列

> **實務上為什麼會遇到**：高頻讀的小物件要對齊 cache line 嗎？直覺回答「要」，但如果物件只有 24 bytes，對齊等於每條 line 浪費一大半，整體 cache footprint 反而變大。這題用確定性的快取模型親手驗證這個取捨。

常數 `kCacheLine = 64`。本題全部用 `solution.hpp` 的介面，快取模型用 `_shared/08_cache_sim.hpp`（`#include "08_cache_sim.hpp"`，其中 `Cache` 請直接讀它的介面註解）。

## Stage 1 — 包住任意 allocator，對齊 cache line

`CacheAlignedWrapper<Upstream>`：`Upstream` 只提供 `void* allocate(std::size_t)`（回傳值對齊到 8，除此之外你**不可以假設**任何狀態）。

- `allocate(bytes)`：回傳的指標對齊到 64，且 `[p, p + bytes)` 完全落在 upstream 配給你的範圍內。`bytes == 0` 回傳 `nullptr`。
- 每次呼叫對 upstream 只要求一次，要求的大小不能超過 `bytes + 63 + sizeof(void*)`。
- `overhead_bytes()`：目前為止向 upstream 多要的 bytes 總和（超出使用者要求的部分）。

## Stage 2 — 支援釋放

`deallocate(p)`：把 upstream 當初給的**原始指標**還給 `Upstream::deallocate(void*)`。`nullptr` 不做事。原始指標存在哪裡？只有 `p` 的時候要怎麼把它找回來？

## Stage 3 — 緊密排列 `PackedAllocator`

把 cache line 對齊的條件放寬：物件之間可以共用 cache line，但每個物件都要跨最少條 line。

`PackedAllocator(std::span<std::byte> heap)`：在 `heap` 上做 bump allocation，每個物件前面放一個 8-byte 的 **size header**：

- `allocate(bytes)`：物件對齊到 8；header 緊貼在物件起點前面。若 `bytes == 0` 或 heap 不夠就回傳 `nullptr`。
- 物件跨越的 cache line 數必須等於 `⌈bytes / 64⌉`。做不到就把物件往後移到下一條 line 的起點（header 可以落在前一條 line 的尾端）。
- `size_of(p)`：回傳當初要求的大小。`used_bytes()`：目前的使用量（高水位）。

line 的邊界要用**真實位址**算（`heap` 的起點不一定對齊到 64）。

## Stage 4 — 實驗

實作 `measure_placement(addresses, sizes, order, cache)`：依 `order` 的順序，對每個物件做一次整段存取（`Cache::access_range`），回傳 `PlacementStats`：

- `lines_per_access`：一次存取平均跨幾條 cache line。
- `misses`：這段過程在 `cache` 上**新增**的 miss 數。

測試會用你的 Stage 1、3 把物件排好，比較「對齊」與「緊密」兩種擺法在同一個快取上的表現。

## 面試追問

1. Stage 1 的做法平均浪費多少 bytes？物件越小，這個比例越糟，為什麼 `aligned_alloc(64, 8)` 這種用法要小心？（作業 Write-up 1）
2. Stage 4 測試的情境裡，對齊的版本 miss 反而比較多。解釋原因：什麼時候對齊會贏、什麼時候緊密排列會贏？
3. `PackedAllocator` 的 header 不放物件前面，放在哪裡還行得通？（作業 Write-up 2）如果 allocator 改成 `free(p, size)` 介面，header 還需要嗎？（作業 Write-up 3）
