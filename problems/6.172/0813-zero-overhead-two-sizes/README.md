---
title: 零額外空間的雙尺寸配置器（pointer tagging）
unit: 08 · Storage Allocation（HW6）
source: 6.172 HW6 §6 Write-up 4–6（fixed aligned 與 smart allocator）
pdf: materials/6.172/homework/hw06/handout.pdf
kind: code
minutes: 70
---

# 0813 · 零額外空間的雙尺寸配置器

> **實務上為什麼會遇到**：`free(p)` 要知道大小，通常靠 header；但如果 block 夠對齊，指標最低幾個位元永遠是 0，可以拿來存資訊，於是 metadata 完全不用佔空間。V8 的 Smi、CPython 的 tagged pointer、各種 lock-free 結構的 tag，都是這招。

heap 是一段對齊 64 的 `std::span<std::byte>`，在其上做 bump allocation 加 free list，全程不存任何 header。本題只處理兩種物件：64 bytes（large）與 32 bytes（small）。

## Stage 1 — 固定大小的 cache-aligned 配置器（Write-up 4）

`FixedAligned64(heap)`：只有 64-byte 物件，全部對齊到 64。

- `allocate()`：優先拿 free list（LIFO），否則從 heap 切；heap 用完回傳 `nullptr`。
- `deallocate(p)`：放回 free list，`nullptr` 不做事。
- `used_bytes()`：bump 指標的位置。全程不需要 header，所以 n 次配置（沒有釋放）剛好用 `64 * n` bytes。

## Stage 2 — Tagged pointer（Write-up 5）

`SmartPtr`：跟 `void*` 一樣大（`sizeof` 相同），把「是不是 small 物件」存在位址的最低位元。

- `SmartPtr::make(address, is_small)`、`get()`（取回真實位址）、`is_small()`、`explicit operator bool`（是否非空，`SmartPtr{}` 為空）、`operator==`。
- `address` 至少對齊到 32，所以最低 5 個位元永遠空著。

## Stage 3 — 兩種大小，不多花一個 byte（Write-up 5–6）

`SmartAllocator(heap)`：

- `allocate(bytes)`：`1..32` 給 small、`33..64` 給 large；其他大小或空間不足回傳空的 `SmartPtr`。
- `deallocate(SmartPtr)`：只靠指標上的 tag 就知道大小，放回對應的 free list。
- small 物件必須符合「某一端點落在 cache line 邊界」：起點 ≡ 0 或 32 (mod 64)；large 物件起點對齊 64。
- 兩條 free list 都優先於 bump，且都是 LIFO。

**不要浪費空間**：當 bump 指標在 line 中間（≡ 32 mod 64）又要配置 large 物件時，要先把指標推到下一條 line，那半條 line 的 32 bytes **放進 small 的 free list**，之後的 small 配置會先用它。

`used_bytes()`：bump 指標的位置。

## 面試追問

1. Write-up 4：固定 64 bytes 的配置器為什麼連 size header 都不需要？free list 的 next 指標存在哪？（比對 0812 的 packed allocator）
2. tagged pointer 的前提是對齊。如果想在指標上塞 16 bit 的 generation，有哪些辦法？（提示：arm64 的 top-byte ignore、x86-64 的 canonical address）
3. Stage 3 的 small free list 為什麼也能重用 large 那半條 line？這樣之後 small 跟 large 的 block 能合併嗎？（下一題 0814）
