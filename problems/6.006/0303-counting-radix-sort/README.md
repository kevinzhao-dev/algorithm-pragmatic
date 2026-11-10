---
title: Counting、Tuple 與 Radix Sort
unit: 03 · Hashing & Linear Sorting（PS3）
source: 6.006 Lecture 5, Recitation 5
pdf: materials/6.006/recitations/rec05.pdf
read: materials/6.006/lectures/lec05.pdf
kind: code
minutes: 75
---

# 0303 · Counting、Tuple 與 Radix Sort

> **實務上為什麼會遇到**：資料庫對整數欄位排序、後綴陣列的建構、GPU 上的排序管線，都用 radix sort，因為鍵的範圍有界時它不受 `Ω(n log n)` 的比較下界限制。一個穩定的底層排序是這整組工具的關鍵。

所有排序都是 **in-place 的介面**：呼叫後 `items` 變成排好的結果（內部可以用額外的 buffer）。不可以用比較排序（`std::sort` 等）。

**元素可能是 move-only、可能沒有 default constructor**，整個實作不能依賴 copy 或預設建構。

## Stage 1 — Counting Sort

`counting_sort(items, u, key)`：`key(item)` 回傳 `[0, u)` 的整數。

- **穩定**：key 相同的元素維持原本順序。
- 時間 O(n + u)。`key` 的呼叫次數要是 O(n)。

## Stage 2 — Tuple Sort

`tuple_sort(items, digits, base, digit)`：每個元素有 `digits` 個欄位，`digit(item, i)` 回傳第 `i` 個欄位（`[0, base)`）。**`i = 0` 是最不重要的欄位**，由此依字典序排序（最重要的欄位是 `digits - 1`），且穩定。

搞懂為什麼必須從最不重要的欄位開始排，而不是反過來。

## Stage 3 — Radix Sort

`radix_sort(items, key)`：`key(item)` 回傳任意 `uint64_t`（**可以接近 `2^64 - 1`**），依 key 遞增排序。

- 以 base-n（`n` 為元素數）拆位數，時間 O(n · ⌈log_n(max + 1)⌉)。key 上界是 n 的多項式時，就是 O(n)。
- 不可以呼叫比較排序。key 的呼叫次數要是 O(n × 位數)，不能是 O(n log n)。
- 邊界：`n ≤ 1`、所有 key 都相同、最大 key 為 `2^64 - 1`（留意 `max + 1` 會溢位，算次方時也會）。

## Stage 4 — 帶號整數與字串

- `sort_signed(values)`：完整 `int64_t` 範圍，線性時間（`INT64_MIN` 不能用取負數的方式處理）。
- `sort_equal_length(strings)`：所有字串長度相同，依 byte 值（`unsigned char`）字典序排序，O(n × L)。字串要 move，不要 copy。

## 面試追問

1. Radix sort 為什麼一定要用穩定的 subroutine？把它換成不穩定的會壞在哪？
2. 實務上會選 base-256 或 base-65536 而不是 base-n，為什麼？代價是什麼？
3. 「radix sort 是 O(n)」這句話成立需要什麼前提？如果 key 是 Θ(n) bits 的大整數呢？
