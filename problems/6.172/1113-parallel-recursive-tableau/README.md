---
title: 遞迴與平行 Tableau（k²-way 的 work 與 span）
unit: 11 · Caching（HW8）
source: 6.172 HW8 §2.2 Write-up 3（k²-way work／span）
pdf: materials/6.172/homework/hw08/handout.pdf
read: materials/6.172/lectures/lec08.pdf
kind: code
minutes: 70
sanitizer: tsan
---

# 1113 · 遞迴與平行 Tableau

> **實務上為什麼會遇到**：把 DP 變成平行程式時，真正的限制是相依關係：同一條反對角線上的區塊可以同時算。切割成四象限，或者更細的 k² 塊，會改變 span，也改變能吃到多少核心。

延續 1112 的 2N−1 壓縮陣列與 `f(A(i-1,j-1), A(i,j-1), A(i-1,j))`，`init` 仍然寫到第一列和第一行。`a` 的介面同 1112。

因為 `sanitizer: tsan`，這題的功能測試跑在 ThreadSanitizer 下，你的平行版本要真的沒有 data race。

## Stage 1 — 遞迴（四象限）

`recursive_tableau(a, n, init, f)`：把還沒算的格子範圍（第 1..n−1 列、第 1..n−1 行）遞迴切成四個象限：左上、右上、左下、右下，依這個順序做。列數或行數為 1 就不再切那一維（`mid` 取 `begin + len/2`）。回傳 `A(n−1, n−1)`。

## Stage 2 — 平行

`parallel_tableau(a, n, init, f)`：左上象限做完之後，**右上和左下可以同時做**（用 `harness/forkjoin.hpp` 的 `practice::fork2`），全部做完才做右下。可以在區塊夠小時（例如邊長 ≤ 32）切回序列版本。

結果必須與序列版完全相同。想想為什麼兩個平行的象限不會寫到同一格、也不會讀到正在被寫的格子（1112 的對應方式幫了大忙）。

## Stage 3 — k²-way 的 work 與 span（Write-up 3）

`tableau_work_span(n, k)`：`n = k^h`，把 `n×n` 切成 k² 塊邊長 n/k 的小塊，每塊遞迴處理，`n == 1` 時 work 與 span 都是 1。同一條反對角線上的塊可以平行，所以總共有 `2k − 1` 個階段。回傳 `{work, span}`。

- `work`：總格數。
- `span`：遞迴式 `S(n) = (2k−1)·S(n/k)`，`S(1) = 1`。

## 面試追問

1. 把 span 寫成 n 的冪次：`Θ(n^{log_k(2k−1)})`。k 越大 parallelism 越高，代價是什麼？（作業 Write-up 3）
2. 如果把 k² 塊裡同一條反對角線的塊全部 spawn，再全部 sync，跟兩兩用 `fork2` 差在哪裡？
3. 這題 TSan 是 clean 的，那 determinacy race 呢？如果 `f` 有內部共享狀態，TSan 一定抓得到嗎？
