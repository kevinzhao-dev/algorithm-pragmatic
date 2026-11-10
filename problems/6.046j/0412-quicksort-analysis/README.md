---
title: Quicksort 分析（用實驗驗證高機率界）
unit: 04 · Amortization 與 Randomization（PS4）
source: 6.046J PS4 Problem 4-2
pdf: materials/6.046j/psets/ps04/questions.pdf
answer: materials/6.046j/psets/ps04/solutions.pdf
read: materials/6.046j/lectures/lec06.pdf, materials/6.046j/recitations/rec04.pdf
kind: code
minutes: 100
---

# 0412 · Quicksort 分析

> **實務上為什麼會遇到**：「平均 O(n log n)」不夠讓人安心，線上系統更在乎尾端：多少機率會超過某個成本？這題把高機率界寫成可以量的實驗，再用它導出 introsort 的防線。

原題是證明，這題把每個論點變成可執行的檢查。證明本身放在 `面試追問`。

## Stage 1 — 好的 Pivot

`good_pivot_count(m, rank)`：大小 `m` 的子陣列中，元素 `x` 的排名是 `rank`（0-based）。pivot 均勻地落在 `m` 個位置上。回傳有幾種 pivot 是「好的」：pivot 就是 `x`，或 `x` 下一次所在的子陣列大小 `≤ 3m/4`（用整數比較 `4·size ≤ 3m`，不要用浮點數）。

測試會枚舉所有 `m ≤ 800` 和所有排名，驗證「至少一半的 pivot 是好的」這個引理。

## Stage 2 — 帶追蹤的 Quicksort

`traced_quicksort(values, rng)`：`values` 元素互異，排序完成，回傳 `QuicksortTrace`：

- `pivot_comparisons[i]`：**起始位置**為 `i` 的元素，總共和 pivot 比較過幾次（它自己當 pivot 的那次不算）。排序時元素會被搬動，要記得它原本在哪。
- `total_comparisons`：總比較次數，等於上面的總和。

規則：每次 partition，pivot 均勻隨機，其餘每個元素**恰好和 pivot 比較一次**。測試用 `Counted` 確認 `total_comparisons` 和真實比較次數一模一樣，也會比對每個元素的平均比較次數是否符合 `H_i + H_{n+1-i} - 2`。

## Stage 3 — 量測高機率界

令 `c = log_{4/3} 2`。

- `element_bound(n, α)`：`3(α + 1 + c) · lg n`。單一元素的比較次數不超過這個值的機率至少 `1 − n^{−(α+1)}`。
- `total_bound(n, α)`：`n · element_bound(n, α)`。總比較次數不超過這個值的機率至少 `1 − n^{−α}`（union bound）。

注意：`3(α + c)` 的寫法（解答 PDF）在 union bound 上差一個 `+1`，這裡請照上面的版本做，理由放進面試追問。

`measure_bound(n, trials, alpha, rng)`：跑 `trials` 次（每次重新打亂），回傳 `BoundReport`：跑了幾次、超過 element bound 幾次、超過 total bound 幾次、平均總比較次數。

## Stage 4 — 實務延伸：Introsort 的防線

如果亂數產生器壞掉了，或對手可以預測 pivot，Stage 2 就會變成 Θ(n²)。`guarded_sort(values, rng)`：

- 分割前先看深度，遞迴深度超過 `2⌈lg n⌉` 時，對這段改用 heapsort（`std::make_heap`、`std::sort_heap` 即可）。
- `values` 可以有重複元素。不必維護 trace。
- 即使 `rng` 永遠回傳 0，比較次數也要 `O(n log n)`。

## 面試追問

1. 證明 Stage 1 的引理：為什麼 pivot 落在中間那段時，兩邊都不超過 `3m/4`？
2. 從引理導出 element bound：什麼是「成功的呼叫」？為什麼需要 `c lg n` 次成功？為什麼要 `3(α + 1 + c) lg n` 次擲幣？
3. 解答 PDF 為什麼少了 `+1`？把 union bound 的每一步寫出來。
4. 深度上限為什麼選 `2 lg n`？選更大或更小會有什麼影響？（對照 `std::sort`。）
