---
title: N-Queens 從循序到平行
unit: 05 · Fork-Join 與 Reducer（HW4）
source: 6.172 HW4 §3.1–3.2（Write-up 1–6）
pdf: materials/6.172/homework/hw04/handout.pdf
read: materials/6.172/lectures/lec03.pdf, materials/6.172/lectures/lec06.pdf
kind: code
sanitizer: tsan
minutes: 140
---

# 0512 · N-Queens 從循序到平行

> **實務上為什麼會遇到**：回溯搜尋是最常見的平行化入門，而「把結果累積到共用的 list」是最常見的第一個 race。解法永遠是同一招：每個平行分支自己累積，join 之後再合併。

## Stage 1 — ResultList

`template <typename T> class ResultList`：擁有自身節點的單向串列。

| 介面 | 要求 |
| --- | --- |
| `push_back(v)` | O(1) |
| `merge(other)` | O(1)：把 `other` 全部節點接到尾端，`other` 變成空的 |
| `size()` / `empty()` | O(1) |
| `to_vector()` | 依序複製出元素 |
| 解構 | 釋放所有節點，不可用遞迴 |
| 移動 | 支援 move 建構／指派，來源變空 |

邊界：`merge` 自己或空串列、把元素 merge 進空串列。

## Stage 2 — 位元回溯

把棋盤壓進 `std::uint64_t`（Bentley 的 packing）：第 `r` 列的皇后所在欄佔 bits `[4r, 4r+4)`，所以 `n <= 16`。`using Board = std::uint64_t;`，`board_column(b, r)` 與 `board_place(b, r, c)` 都要實作。

用 Lecture 3 的三個 `n`-bit 向量（`down`、`left`、`right`，`left`／`right` 每往下一列要左／右位移一格，只留 `n` 位元）：可用位置 = `~(down|left|right) & all`，用 `x & -x` 取最低位。`n` 之外的位元要記得裁掉。

`queens_serial(n)`（`1 <= n <= 16`）回傳所有解的 `ResultList<Board>`。

## Stage 3 — 無 race 的平行版

`queens_parallel(exec, n, serial_depth)`：`exec` 是 `exec::Meter` 或 `exec::ForkJoin`（`_shared/01_exec.hpp`）。

- 每個平行分支把解放到**自己的** `ResultList`，`fork2` 回來後才 `merge` 成一個結果（HW4 的 temp list 策略）。
- `row < serial_depth` 的節點用 `fork2`（可用的皇后位置要二分成兩堆遞迴）；`row >= serial_depth` 就走一般遞迴（coarsening）。
- 每拜訪一個搜尋節點（呼叫一次回溯函式，含 `row == n` 的葉節點）`exec.charge(1)`。
- 用 `exec::Meter` 時完全可重現；用 `exec::ForkJoin` 由 TSan 保證沒有 race。

## Stage 4 — Coarsening 的代價（Write-up 6）

`serial_depth = 0`（完全循序）的 `spawns()` 為 0，`serial_depth = 1` 只有第一列 fork（`n-1` 次），並且所有深度算出的解集合相同。

## 面試追問

1. Write-up 1：為什麼 `left`／`right` 只要 `n` 位元就夠，而不是 `2n-1`？
2. 兩個單向串列 concat 為什麼需要 tail 指標？少了它，merge 的成本會變成什麼？對平行版本的 work 有什麼影響？（HW4 Write-up 4）
3. 平行版比循序版慢，通常是因為什麼？你會怎麼量？
