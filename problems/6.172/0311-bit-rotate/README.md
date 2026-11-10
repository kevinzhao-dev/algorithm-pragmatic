---
title: Bit Array 與快速旋轉
unit: 03 · Bit Hacks（Project 1）
source: 6.172 Project 1: Bit Hacks
pdf: materials/6.172/projects/p1/handout.pdf
read: materials/6.172/lectures/lec03.pdf, materials/6.172/projects/p1/handout.pdf
kind: project
minutes: 300
---

# 0311 · Bit Array 與快速旋轉

> **實務上為什麼會遇到**：列式資料庫的 bit-packed column、壓縮點陣圖索引、ring buffer 的位元移位，都要在「不是 byte 對齊」的位置做區段搬移。這題把 word-level parallelism（一次處理 64 位元）用在真正要命的地方。

專案分兩個檔案：`bit_array.hpp` 放 `BitArray`，`bit_packed_vector.hpp` 放 `BitPackedVector`，`solution.hpp` 把兩個 include 進來。不要用 `__builtin_bitreverse64` 這類內建函式，一次翻轉 64 位元要自己用遮罩做（可以看 0301）。

## Stage 1 — BitArray 基本功

`BitArray(n)`（全為 0）、`size()`、`get(i)`、`set(i, v)`，索引超出範圍丟 `std::out_of_range`。`from_string("1011")` 建立陣列（字元 0 是索引 0），含其他字元丟 `std::invalid_argument`。`to_string()` 反向轉回字串。

## Stage 2 — 跨 word 的欄位存取

- `read_bits(offset, count)`：`count ∈ [0, 64]`，把 `[offset, offset+count)` 讀成一個整數，`offset` 那一位成為結果的 bit 0。
- `write_bits(offset, count, value)`：把 `value` 的低 `count` 位寫回，其他位元不動。
- 範圍超出 `size()` 丟 `std::out_of_range`。

這是後面一切的基礎。`count == 64` 且未對齊是最容易寫錯的情況。

## Stage 3 — 正確的 rotate

`rotate(offset, length, right_amount)`：旋轉 `[offset, offset+length)` 這一段，`right_amount > 0` 表示朝索引變大的方向循環移動（每個 bit 從 `p` 移到 `offset + ((p-offset+right_amount) mod length)`），`< 0` 朝索引變小的方向。其他位元不動。`right_amount` 可以是任何 `std::ptrdiff_t` 值（包含 `PTRDIFF_MIN`，取負會溢位）。範圍超出丟 `std::out_of_range`。

這一關只要對，可以用簡單的 O(length) 方法。

## Stage 4 — 用 word-level parallelism 加速

`rotate` 要比固定的 baseline（`bit-by-bit` 三次反轉，直接用 `get`/`set` 級的逐位元操作）快至少 **16 倍**。Project 1 handout 第 7 頁列了三種思路：輔助陣列、循環替換、三次反轉（`(aᴿbᴿ)ᴿ = ba`）；反轉可以用 word-level：一次取 64 位元、翻轉、對調兩端。自己量看看哪一種最快。

提示：先把 `read_bits`/`write_bits` 做對，rotate 就只是「把位元反轉」的組合。

## Stage 5 — 實務延伸：Bit-packed column

`BitPackedVector(width, capacity)`：容量固定的整數欄，每個元素佔 `width` 位元（`1..64`，不合法丟 `std::invalid_argument`）。

- `push_back(v)` / `get(i)` / `set(i, v)`：`v` 放不進 `width` 位元丟 `std::invalid_argument`；索引超出丟 `std::out_of_range`；滿了 `push_back` 丟 `std::length_error`。
- `insert(i, v)`（`0 <= i <= size()`）、`erase(i)`（`0 <= i < size()`）：用 `BitArray::rotate` 在位元層級搬移後續元素，不可逐元素移動。

## 面試追問

1. 三次反轉和循環替換都只用 O(1) 額外空間，為什麼快取行為差那麼多？
2. 如果要支援 big-endian 機器，你的 `read_bits` 哪裡要改？
3. Handout 第 8 頁說測試涵蓋率佔了很大的成績比重。你會挑哪 5 個 rotate 測試案例，能最快抓出別人的 bug？
