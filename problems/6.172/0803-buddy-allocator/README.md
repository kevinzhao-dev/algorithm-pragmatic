---
title: Binned Free Lists 與 Buddy Coalescing
unit: 08 · Storage Allocation（HW6）
source: 6.172 Lecture 11 Allocation for Binned Free Lists、Coalescing
pdf: materials/6.172/lectures/lec11.pdf
kind: code
minutes: 90
---

# 0803 · Binned Free Lists 與 Buddy Coalescing

> **實務上為什麼會遇到**：Linux kernel 的 page allocator 就是這套 buddy 系統。問題的本質很典型：bin 用完時要把大塊切開（split），之後又要在不增加太多成本的情況下把相鄰的塊合併回來（coalesce），否則 external fragmentation 會把記憶體耗光。

一個只用位移（offset）的 allocator，不碰真實記憶體：整個 arena 有 2^`max_order` 個 unit。order k 的 block 長度是 2^k unit，起點一定是 2^k 的倍數。用 Lecture 11 的 binned 配置法實作：

## Stage 1 — 配置與切分

`allocate(units)`：先算出放得下 `units` 的最小 order k。

- bin k 有 free block 就直接用。
- 沒有的話，找最小的非空 bin k′ > k，把那塊一路切半：每切一次，把**高位那半**放進較小的 bin，繼續切**低位那半**，直到 order k，把低位的 k 階 block 回傳。
- arena 裡沒有夠大的 block 時回傳 `std::nullopt`。

`units == 0` 丟 `std::invalid_argument`。`free_blocks(order)` 回傳該 bin 的 block 數；`free_units()` 回傳目前閒置的 unit 總數。

每個 bin 要用能 O(log n) 插入、刪除指定 offset 的結構。

## Stage 2 — 釋放與合併

`free(offset)`：`offset` 是 `allocate` 回傳過、尚未釋放的值。釋放時如果它的 **buddy**（同 order、相鄰且一起才能湊成更大 block 的那一塊）也是閒置的，兩塊就立刻合併，並繼續往上檢查，直到不能再合併。

合併要是 O(log n)，不可以線性掃描。全部釋放之後，不管順序如何，arena 必須回到一個完整的 max-order block。

## Stage 3 — 碎片指標

- `largest_free_block()`：目前最大閒置 block 的 unit 數（沒有閒置就回傳 0）。
- `external_fragmentation()`：`1 - largest / free_units`，全部配置時定義為 0。

這一關的測試會構造幾種配置模式，確認指標算得對。想想棋盤格的情況為什麼 buddy 系統也幫不上忙。

## 面試追問

1. 為什麼 buddy 的 offset 可以用一行位元運算算出來？這跟「block 起點必須是自己長度的倍數」有什麼關係？
2. Lecture 11 的分析：binned free list 的外部碎片上界是多少倍的最大使用量？為什麼 coalescing 實務上很有用，但沒有改變最壞情況的上界？
3. Linux 的 buddy allocator 上面還疊了 slab。這兩層各解決什麼問題？
