---
title: Bit Tricks 工具箱
unit: 03 · Bit Hacks（Project 1）
source: 6.172 Lecture 3（Bit Hacks）
pdf: materials/6.172/lectures/lec03.pdf
read: materials/6.172/lectures/lec03.pdf
kind: code
minutes: 80
---

# 0301 · Bit Tricks 工具箱

> **實務上為什麼會遇到**：bitmap 索引、位元壓縮、hash table 的 probing、排程器的 ready mask，全都靠這些招式。面試官問「怎麼不用迴圈算 popcount」的時候，答案不是背公式，是知道為什麼可行。

所有函式放在 `solution.hpp`。這題是練手法，不是練呼叫標準函式庫：不要用 `std::popcount`、`std::countr_zero`、`__builtin_*`（測試用它們當對照）。「no branch」的要求測試抓不到，請自律：判斷式都用算術消去。

## Stage 1 — Mask 與欄位

`set_bit`、`clear_bit`、`toggle_bit`（`k < 64`）、`extract_field(x, lo, len)`、`insert_field(x, lo, len, value)`。
`len` 可以從 `0` 到 `64`，`lo + len <= 64`。`insert_field` 只動 `[lo, lo+len)`，`value` 多餘的高位要忽略。注意 `len == 64` 時別用 `1 << 64`。

## Stage 2 — No-branch

- `min_no_branch(x, y)`：`int64_t` 全範圍都要對（包含 `x - y` 會溢位的情況）。
- `abs_no_branch(x)`：`x != INT64_MIN`。
- `mod_add_no_branch(a, b, n)`：`0 <= a, b < n`，回傳 `(a + b) mod n`。`n` 可以到 `2^63`。
- `merge_no_branch(a, na, b, nb, out)`：把兩個已排序陣列合併到 `out`，比較步驟不用分支。

## Stage 3 — Power 與計數

| 函式 | 規格 |
| --- | --- |
| `ceil_pow2(x)` | 不小於 `x` 的最小 2 的次方；`x == 0` 回傳 `1`；`x > 2^63` 回傳 `0` |
| `lowest_set_bit(x)` | 只留最低的 1；`x == 0` 回傳 `0` |
| `count_trailing_zeros(x)` | `x != 0`；用 de Bruijn 乘法查表（Lecture 3） |
| `popcount_table(x)` | 256 項查表 |
| `popcount_parallel(x)` | SWAR：相鄰位元加總，然後兩兩加、四四加…… |
| `reverse_bits(x)` | 把 64 位元前後翻轉，同樣用分治遮罩 |

## Stage 4 — 實務延伸

- `SetBits`：可以 range-for 的型別，`for (unsigned i : SetBits{x})` 依序給出每個 1 的位置（由低到高），複雜度正比於 1 的個數。
- `next_with_same_popcount(x)`（Gosper's hack）：比 `x` 大、`popcount` 相同的最小整數；沒有這樣的 64 位元整數時回傳 `0`。枚舉 `k`-subset 就靠它。

## 面試追問

1. `x & (x - 1)` 和 `x & -x` 各做了什麼？`-x` 在位元層面是什麼？
2. No-temp swap 用 `^` 的版本，什麼情況下會把資料清成 0？
3. de Bruijn 序列為什麼能把「最低位 1 在第幾位」壓成一張 64 項的表？
