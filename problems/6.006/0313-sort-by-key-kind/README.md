---
title: 依鍵的型態選排序法
unit: 03 · Hashing & Linear Sorting（PS3）
source: 6.006 PS3 Problem 3-3
pdf: materials/6.006/psets/ps3/questions.pdf
answer: materials/6.006/psets/ps3/solutions.pdf
read: materials/6.006/lectures/lec05.pdf, materials/6.006/recitations/rec05.pdf
kind: code
minutes: 50
---

# 0313 · 依鍵的型態選排序法

> **實務上為什麼會遇到**：同一批量測資料，依 ID、依整數欄位、依高精度小數、依只能比大小的資料分別排序時，最好的方法完全不同。資料庫的 query planner 每天都在做這個選擇。

一個資料庫的批次匯入管線拿到 `n` 筆 `Slice`，要依不同的鍵排序。`Slice` 是 **move-only**，移動一下就好，不要複製。建議直接 `#include "../0303-counting-radix-sort/solution.hpp"`，重用你寫好的 `counting_sort`、`tuple_sort`、`radix_sort`。Stage 1–3 都應該是線性（對長度 `L` 的 Stage 1 是 O(n·L)），不要用比較排序；Stage 4 才需要它。

## Stage 1 — 依固定長度字串 ID

`sort_by_core_id(slices)`：每筆的 `core_id` 長度相同，只含 ASCII（0–127），長度 `L` 可能很長（原題設 `L = 16⌈log₄√n⌉ = Θ(log n)`）。依 byte 字典序排序。O(n · L)。

## Stage 2 — 依年齡

`sort_by_age(slices)`：`age` 是 `[0, 800000]` 的整數。原題把 800000 當常數，所以 counting sort 是 O(n)。

但這個函式會被**頻繁呼叫，每次 batch 可能只有幾筆**。如果每次都開一個 800001 格的陣列，Θ(u) 的常數在小批次時會把你拖垮。要在小批次時也快。

## Stage 3 — 依有理數厚度

`sort_by_thickness(slices)`：`thickness_numerator` 是整數 `m`，厚度是 `m / n³`（`n` 是這批的筆數），`0 ≤ m ≤ 4n³`。依厚度遞增排序。

- `m` 可以到 `4n³`，已超過 `2^53`；絕對不可以先轉成 `double` 再比，鄰近的 `m` 會被視為相同。
- `n` 不大於 `10^6`，`m` 在 `uint64_t` 範圍內。O(n)。

## Stage 4 — 只能比較的鍵

`sort_opaque(slices, less)`：這個鍵（例如 `memory`）只能透過 `less(a, b)` 比大小，連 `memory` 欄位你都不該去看。O(n log n) 次比較；這裡比較排序就是最佳解。

## 面試追問

1. Stage 1 真正的複雜度是什麼？`L = Θ(log n)` 時，一次處理一個 byte 的 LSD 還算線性嗎？要怎麼才算？
2. Stage 2 的小批次陷阱，在你的 base-n radix sort 上是怎麼避開的？如果對方堅持用 counting sort，你會怎麼改？
3. Stage 3 為什麼不能用浮點？`n` 再大十倍時，`uint64_t` 夠用嗎？
