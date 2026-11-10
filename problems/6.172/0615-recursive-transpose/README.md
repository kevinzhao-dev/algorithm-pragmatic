---
title: 遞迴平行轉置
unit: 06 · Work/Span 理論（HW5）
source: 6.172 HW5 Write-up 5（CLRS 27.2-5）
pdf: materials/6.172/homework/hw05/handout.pdf
read: materials/6.172/lectures/lec08.pdf
kind: code
sanitizer: tsan
minutes: 90
---

# 0615 · 遞迴平行轉置

> **實務上為什麼會遇到**：tensor 的 layout 轉換（channel-first ↔ channel-last）、影像旋轉、FFT 的 bit-reversal 都要做 transpose。分治法除了能平行，還有天生的 cache-oblivious 行為，同一份程式碼在 L1 和 L2 都表現得不錯。

平行迴圈版的轉置已經在 0501 做過。這題用 **分治**，把 `n×n` 分成四塊 `A11 A12 / A21 A22`：A11、A22 各自遞迴就地轉置，A12 與 A21 互相對調（對調時順便轉置）。三件事彼此獨立，可以同時做，這就是它 span 小的原因。

演算法寫成 `template <typename Exec>`，用 `exec::Meter`（確定性算 work／span，見 `_shared/01_exec.hpp`）驗證複雜度，用 `exec::ForkJoin` 由 TSan 把關。每交換一對元素、每複製一個元素各 `exec.charge(1)`（`fork2` 本身會再 charge 1，別重複算）。

## Stage 1 — 就地轉置

`transpose_square(exec, grid, n, cutoff = 1)`：`grid` 是 `n×n` row-major（`grid.size() == n*n`）。邊長 `<= cutoff` 的子問題改用循序迴圈。

要求（`cutoff = 1`）：

- Work 恰好是「交換次數 `n(n-1)/2` + fork 次數」。
- Span 是 `O(lg n)`：測試要求不超過 `8·⌈lg n⌉ + 16`。

提示：對調兩個長方形的那個遞迴也要切半，不然 span 會是 `Θ(n)`。

## Stage 2 — 長方形、離位

`transpose_copy(exec, src, rows, cols, dst, cutoff = 1)`：把 `rows × cols` 的 `src`（row-major）轉置到 `dst`（`cols × rows`，row-major）。遞迴切較長的那一邊，面積 `<= cutoff` 時用循序迴圈。Work 是 `rows*cols` + fork 次數；span 不超過 `4·(⌈lg rows⌉ + ⌈lg cols⌉) + 16`。

## 面試追問

1. 寫出 Stage 1 的 work 與 span 遞迴式並解出來（HW5 Write-up 5 要的就是這個）。對調長方形那一支的 span 是多少？
2. 迴圈版（0501）與遞迴版的 work 相同，差別在哪？（提示：cache misses，HW8 會量）
3. `cutoff` 選多少合理？跟 fork 成本和快取大小各有什麼關係？
