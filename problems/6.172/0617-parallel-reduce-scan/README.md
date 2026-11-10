---
title: 平行 Reduce 與 Prefix Scan
unit: 06 · Work/Span 理論（HW5）
source: 6.172 HW5 Write-up 7（CLRS Problem 27-4）
pdf: materials/6.172/homework/hw05/handout.pdf
read: materials/6.172/lectures/lec08.pdf
kind: code
sanitizer: tsan
minutes: 150
---

# 0617 · 平行 Reduce 與 Prefix Scan

> **實務上為什麼會遇到**：`reduce` 和 `scan` 是資料平行的兩塊基石。MapReduce、SQL 的累計欄位、平行 filter／partition 全都建立在它們上面。Scan 特別有意思：它看起來天生循序（每一項都依賴前一項），卻能做到線性的 work 和對數的 span。

`op` 滿足結合律，但**不一定交換**（測試會用字串串接和 2×2 矩陣乘法）：`op(a, b)` 的 `a` 一定在 `b` 左邊。元素型別可預設建構、可複製。

全部寫成 `template <typename Exec>`（`_shared/01_exec.hpp`）：`exec::Meter` 量 work／span，`exec::ForkJoin` 由 TSan 把關。**每呼叫一次 `op` 就 `exec.charge(1)`**（`fork2` 自己會多 charge 1）。

## Stage 1 — P-REDUCE

`p_reduce(exec, x, op)`：回傳 `x[0] ⊗ x[1] ⊗ … ⊗ x[n-1]`，用分治 `fork2`。`x` 為空丟 `std::invalid_argument`；單一元素直接回傳（不呼叫 `op`）。`op` 次數恰為 `n-1`，span `O(lg n)`。

## Stage 2 — P-SCAN-1

`p_scan_1(exec, x, op)`：回傳 prefix 陣列 `y`，`y[i] = x[0] ⊗ … ⊗ x[i]`，空輸入回傳空陣列。依 CLRS 的做法：平行迴圈，每一項各自呼叫 `p_reduce`。`op` 次數恰為 `n(n-1)/2`。

（這就是把循序 scan 一行改成平行的結果：有 race 嗎？沒有；划算嗎？work 從 `Θ(n)` 變 `Θ(n²)`。）

## Stage 3 — P-SCAN-2

`p_scan_2(exec, x, op)`：分治。遞迴算出左半和右半各自的 prefix，然後用 `parallel_for` 把左半的總和併進右半的每一項。`n` 為 2 的次方時 `op` 次數恰為 `(n/2)·lg n`；span `O(lg²n)`。

## Stage 4 — P-SCAN-3（兩趟）

`p_scan_3(exec, x, op)`：CLRS 的兩趟版本。Up 趟把子區間的累積量存進暫存陣列 `t`，Down 趟把「這個區間左邊所有元素的累積量 `v`」往下傳。**`P-SCAN-UP` 回傳什麼、`P-SCAN-DOWN` 兩個遞迴呼叫要傳什麼 `v`，請自己推出來**（提示：證明 `v = x[1] ⊗ … ⊗ x[i-1]`）。`op` 次數不超過 `3n`，span `O(lg n)`（不超過 `6⌈lg n⌉ + 12`）。

## Stage 5 — 實務延伸：平行 filter

`parallel_filter(exec, x, pred)`：回傳 `pred` 為真的元素，保持原順序。用「flags → scan 出輸出位置 → 平行寫入」的三步驟，所以 work `O(n)`、span `O(lg n)`（flag 判斷每個元素 `charge(1)`，寫入輸出每個元素 `charge(1)`）。

## 面試追問

1. 分析 P-SCAN-1、P-SCAN-2、P-SCAN-3 的 work、span、parallelism（HW5 Write-up 7 的 b、c、e），並說明 P-SCAN-3 的正確性（d）。
2. 為什麼 scan 的 work 還是 `Θ(n)`，卻不能比循序版的 work 少？P-SCAN-3 比循序版多做了多少次 `op`？
3. `parallel_filter` 為什麼不能用「每個 worker 一個 vector，最後接起來」？（0513 的問題）
